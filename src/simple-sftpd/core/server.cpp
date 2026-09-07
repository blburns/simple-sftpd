/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simple-sftpd/core/server.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include "simple-sftpd/core/connection_manager.hpp"
#include "simple-sftpd/core/connection.hpp"
#include "simple-sftpd/config/server_config.hpp"
#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/user/user.hpp"
#include "simple-sftpd/security/ip_access_control.hpp"
#include "simple-sftpd/utils/performance_monitor.hpp"
#include "simple-sftpd/utils/file_cache.hpp"
#include "simple-sftpd/security/rate_limiter.hpp"
#include "simple-sftpd/virtual_host/virtual_host_manager.hpp"
#include "simple-sftpd/core/session_tracker.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <algorithm>
#include <filesystem>
#ifndef _WIN32
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#endif

namespace simple_sftpd {

namespace {

std::string defaultVirtualHostsFilePath() {
#ifdef _WIN32
    return "C:\\Program Files\\simple-sftpd\\virtual_hosts.json";
#else
    std::string path = "/etc/simple-sftpd/virtual_hosts.json";
    if (!std::filesystem::exists("/etc/simple-sftpd") && access("/etc/simple-sftpd", W_OK) != 0) {
        const char* home = getenv("HOME");
        path = std::string(home ? home : ".") + "/.simple-sftpd/virtual_hosts.json";
    }
    return path;
#endif
}

} // namespace

FTPServer::FTPServer(std::shared_ptr<FTPServerConfig> config)
    : config_(config), running_(false), server_socket_(-1) {
    LogFormat log_format = LogFormat::STANDARD;
    std::string format_upper = config->logging.log_format;
    std::transform(format_upper.begin(), format_upper.end(), format_upper.begin(), ::toupper);
    if (format_upper == "JSON") {
        log_format = LogFormat::JSON;
    } else if (format_upper == "EXTENDED") {
        log_format = LogFormat::EXTENDED;
    }
    logger_ = std::make_shared<Logger>("", LogLevel::INFO, true, false, log_format);
    connection_manager_ = std::make_shared<FTPConnectionManager>(config, logger_);
    ip_access_control_ = std::make_shared<IPAccessControl>(logger_);
    performance_monitor_ = std::make_shared<PerformanceMonitor>(logger_);
    file_cache_ = std::make_shared<FileCache>(logger_, 1000, std::chrono::seconds(60));
    
    // Initialize rate limiter if enabled
    if (config->rate_limit.enabled) {
        rate_limiter_ = std::make_shared<FTPRateLimiter>(logger_);
        rate_limiter_->setRateLimit(config->rate_limit.max_requests_per_minute);
        rate_limiter_->setConnectionLimit(config->rate_limit.max_connections_per_ip);
    }
    virtual_host_manager_ = std::make_shared<FTPVirtualHostManager>(
        logger_,
        config->security.virtual_hosts_file.empty() ? defaultVirtualHostsFilePath()
                                                    : config->security.virtual_hosts_file);
    session_tracker_ = std::make_shared<SessionTracker>();
}

FTPServer::~FTPServer() {
    stop();
}

bool FTPServer::start() {
    if (running_) {
        return true;
    }
    
    // Create socket (support both IPv4 and IPv6)
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        logger_->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    
    // Enable IPv6 if available (dual-stack)
    int ipv6_only = 0;
    setsockopt(server_socket_, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only));
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_->error("Failed to set socket options: " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(server_socket_, F_GETFL, 0);
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
    
    // Bind socket
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(config_->connection.bind_address.c_str());
    addr.sin_port = htons(config_->connection.bind_port);
    
    if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        logger_->error("Failed to bind socket: " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    struct sockaddr_in bound{};
    socklen_t bound_len = sizeof(bound);
    if (getsockname(server_socket_, (struct sockaddr*)&bound, &bound_len) == 0) {
        listen_port_ = ntohs(bound.sin_port);
    } else {
        listen_port_ = config_->connection.bind_port;
    }
    
    // Listen
    if (listen(server_socket_, config_->connection.max_connections) < 0) {
        logger_->error("Failed to listen on socket: " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }
    
    // Drop privileges if configured (after binding to privileged port)
    dropPrivileges();
    
    // Start connection manager
    if (!connection_manager_->start()) {
        logger_->error("Failed to start connection manager");
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&FTPServer::serverLoop, this);
    
    logger_->info("FTP Server started on " + config_->connection.bind_address + 
                  ":" + std::to_string(listen_port_));
    return true;
}

void FTPServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Close server socket
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    // Stop connection manager
    if (connection_manager_) {
        connection_manager_->stopAllConnections();
        connection_manager_->stop();
    }
    
    // Wait for server thread
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    logger_->info("FTP Server stopped");
}

bool FTPServer::isRunning() const {
    return running_;
}

void FTPServer::serverLoop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket >= 0) {
            // Get client IP address
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            
            // Check IP access control
            if (ip_access_control_ && !ip_access_control_->isAllowed(client_ip)) {
                logger_->warn("Connection rejected from blocked IP: " + std::string(client_ip));
                close(client_socket);
                continue;
            }
            
            // Check rate limiting
            if (rate_limiter_ && !rate_limiter_->isAllowed(client_ip)) {
                logger_->warn("Connection rejected due to rate limit: " + std::string(client_ip));
                close(client_socket);
                continue;
            }
            
            // Record request for rate limiting
            if (rate_limiter_) {
                rate_limiter_->recordRequest(client_ip);
            }
            
            // Check connection limit
            if (connection_manager_->getConnectionCount() >= static_cast<size_t>(config_->connection.max_connections)) {
                logger_->warn("Connection limit reached, rejecting new connection");
                close(client_socket);
                continue;
            }
            
            // Record connection
            if (performance_monitor_) {
                performance_monitor_->recordConnection();
            }
            
            // Handle new connection
            handleConnection(client_socket);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (running_) {
                logger_->error("Accept error: " + std::string(strerror(errno)));
            }
        }
        
        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void FTPServer::handleConnection(int client_socket) {
    int flags = fcntl(client_socket, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
    }
    auto connection = std::make_shared<FTPConnection>(client_socket, logger_, config_, virtual_host_manager_, session_tracker_);
    connection_manager_->addConnection(connection);
    connection->start();
    
    // Note: Connection will remove itself when done
    // For now, we'll let the connection manager handle cleanup
}

void FTPServer::dropPrivileges() {
#ifndef _WIN32
    if (!config_->security.drop_privileges) {
        return;
    }
    
    // Get user and group IDs
    struct passwd* pw = getpwnam(config_->security.run_as_user.c_str());
    struct group* gr = getgrnam(config_->security.run_as_group.c_str());
    
    if (!pw) {
        logger_->warn("User not found: " + config_->security.run_as_user + ", skipping privilege drop");
        return;
    }
    
    if (!gr) {
        logger_->warn("Group not found: " + config_->security.run_as_group + ", skipping privilege drop");
        return;
    }
    
    // Set group ID first (requires root)
    if (setgid(gr->gr_gid) != 0) {
        logger_->error("Failed to set group ID: " + std::string(strerror(errno)));
        return;
    }
    
    // Set user ID (requires root)
    if (setuid(pw->pw_uid) != 0) {
        logger_->error("Failed to set user ID: " + std::string(strerror(errno)));
        return;
    }
    
    logger_->info("Dropped privileges to user: " + config_->security.run_as_user + 
                  ", group: " + config_->security.run_as_group);
#else
    // Privilege dropping not supported on Windows
    (void)config_;
    logger_->warn("Privilege dropping not supported on Windows");
#endif
}

} // namespace simple_sftpd
