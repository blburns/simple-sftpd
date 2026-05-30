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

#include "simple-sftpd/core/connection.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/user/user.hpp"
#include "simple-sftpd/config/server_config.hpp"
#include "simple-sftpd/security/ssl_context.hpp"
#include "simple-sftpd/utils/file_cache.hpp"
#include "simple-sftpd/security/pam_auth.hpp"
#include "simple-sftpd/virtual_host/virtual_host_manager.hpp"
#include "simple-sftpd/virtual_host/virtual_host.hpp"
#include "simple-sftpd/core/session_tracker.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#if defined(__linux__)
#include <sys/sendfile.h>
#elif defined(__APPLE__)
#include <sys/socket.h>  /* sendfile on macOS */
#endif
#if !defined(_WIN32)
#include <sys/mman.h>
#endif
#include <sstream>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <errno.h>
#include <chrono>
#include <thread>
#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

namespace simple_sftpd {

namespace {
uint64_t getDirectorySize(const std::string& path) {
    uint64_t total = 0;
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path,
                std::filesystem::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::error_code ec;
                total += std::filesystem::file_size(entry.path(), ec);
            }
        }
    } catch (const std::exception&) {}
    return total;
}
} // namespace

FTPConnection::FTPConnection(int socket, std::shared_ptr<Logger> logger, std::shared_ptr<FTPServerConfig> config,
                             std::shared_ptr<FTPVirtualHostManager> vhost_manager,
                             std::shared_ptr<SessionTracker> session_tracker)
    : socket_(socket), logger_(logger), config_(config), user_manager_(),
      active_(false), authenticated_(false), current_user_(nullptr), current_directory_("/"),
      ssl_enabled_(false), ssl_active_(false), ssl_(nullptr), data_ssl_(nullptr),
      passive_listen_socket_(-1), data_socket_(-1), transfer_type_("A"), protection_level_("C"),
      active_mode_port_(0), active_mode_enabled_(false), resume_position_(0),
      virtual_host_manager_(vhost_manager), current_virtual_host_(nullptr), session_tracker_(session_tracker) {
    // User file path: from config (persistent storage) or default
    std::string user_file = config->security.user_file;
    if (user_file.empty()) {
        #ifdef _WIN32
        user_file = "C:\\Program Files\\simple-sftpd\\users.json";
        #else
        user_file = "/etc/simple-sftpd/users.json";
        if (!std::filesystem::exists("/etc/simple-sftpd") && access("/etc/simple-sftpd", W_OK) != 0) {
            const char* home = getenv("HOME");
            user_file = std::string(home ? home : ".") + "/.simple-sftpd/users.json";
        }
        #endif
    }
    user_manager_ = std::make_shared<FTPUserManager>(logger_, user_file);
    
    // Add default test user for development/testing (only if no users loaded)
    if (user_manager_->listUsers().empty()) {
        auto test_user = std::make_shared<FTPUser>("test", "test", "/tmp");
        user_manager_->addUser(test_user);
    }
    
    // Add anonymous user if allowed (only if not already exists)
    if (config->security.allow_anonymous && !user_manager_->getUser("anonymous")) {
        auto anon_user = std::make_shared<FTPUser>("anonymous", "", "/tmp");
        user_manager_->addUser(anon_user);
    }
    
    // Initialize PAM if enabled
    if (config->security.enable_pam) {
        pam_auth_ = std::make_shared<PAMAuth>(logger_);
        if (pam_auth_->isAvailable()) {
            logger_->info("PAM authentication enabled");
        } else {
            logger_->warn("PAM authentication requested but not available");
        }
    }
    
    // Initialize SSL if configured
    if (!config->security.ssl_cert_file.empty() && !config->security.ssl_key_file.empty()) {
        ssl_context_ = std::make_shared<SSLContext>(logger_);
        if (ssl_context_->initialize(config->security.ssl_cert_file, config->security.ssl_key_file, 
                                     config->security.ssl_ca_file, config->security.require_client_cert,
                                     config->security.ssl_client_ca_file)) {
            ssl_enabled_ = true;
            logger_->info("SSL/TLS enabled for connection");
        } else {
            logger_->warn("Failed to initialize SSL context");
        }
    }
}

FTPConnection::~FTPConnection() {
    stop();
}

void FTPConnection::start() {
    if (active_) {
        return;
    }
    
    active_ = true;
    client_thread_ = std::thread(&FTPConnection::handleClient, this);
    logger_->info("FTP connection started");
}

void FTPConnection::stop() {
    if (!active_) {
        return;
    }
    if (session_tracker_ && session_registered_) {
        std::string hostname = current_virtual_host_ ? current_virtual_host_->getHostname() : "";
        session_tracker_->unregisterSession(username_, hostname);
        session_registered_ = false;
    }
    active_ = false;
    // Cleanup SSL
    if (ssl_context_ && ssl_) {
        ssl_context_->shutdownSSL(ssl_);
        ssl_context_->freeSSL(ssl_);
        ssl_ = nullptr;
    }
    if (ssl_context_ && data_ssl_) {
        ssl_context_->shutdownSSL(data_ssl_);
        ssl_context_->freeSSL(data_ssl_);
        data_ssl_ = nullptr;
    }
    
    closeDataSocket();
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
    if (client_thread_.joinable()) {
        client_thread_.join();
    }
    logger_->info("FTP connection stopped");
}

bool FTPConnection::isActive() const {
    return active_;
}

void FTPConnection::handleClient() {
    // Send welcome message
    sendResponse("220 Welcome to Simple Secure FTP Daemon");
    
    std::string line;
    while (active_ && (line = readLine()).length() > 0) {
        if (line.empty()) {
            continue;
        }
        
        // Parse command
        std::istringstream iss(line);
        std::string command;
        std::string argument;
        
        iss >> command;
        std::getline(iss, argument);
        
        // Trim whitespace from argument
        argument.erase(0, argument.find_first_not_of(" \t"));
        argument.erase(argument.find_last_not_of(" \t") + 1);
        
        // Convert command to uppercase
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);
        
        logger_->debug("Received command: " + command + (argument.empty() ? "" : " " + argument));
        
        // Handle commands
        if (command == "USER") {
            handleUSER(argument);
        } else if (command == "PASS") {
            handlePASS(argument);
        } else if (command == "QUIT") {
            handleQUIT();
            break;
        } else if (command == "NOOP") {
            sendResponse("200 NOOP command successful");
        } else if (command == "SYST") {
            sendResponse("215 UNIX Type: L8");
        } else if (command == "FEAT") {
            sendResponse("211-Features:");
            if (virtual_host_manager_ && !virtual_host_manager_->listVirtualHosts().empty()) {
                sendResponse(" HOST");
            }
            if (ssl_enabled_) {
                sendResponse(" AUTH TLS");
                sendResponse(" PBSZ");
                sendResponse(" PROT");
            }
            sendResponse("211 End");
        } else if (command == "AUTH") {
            handleAUTH(argument);
        } else if (command == "PBSZ") {
            handlePBSZ(argument);
        } else if (command == "PROT") {
            handlePROT(argument);
        } else if (command == "HOST") {
            handleHOST(argument);
        } else if (authenticated_) {
            // Commands that require authentication
            if (command == "PWD" || command == "XPWD") {
                handlePWD();
            } else if (command == "CWD" || command == "XCWD") {
                handleCWD(argument);
            } else if (command == "LIST" || command == "NLST") {
                handleLIST(argument);
            } else if (command == "PASV") {
                handlePASV();
            } else if (command == "TYPE") {
                handleTYPE(argument);
            } else if (command == "SIZE") {
                handleSIZE(argument);
            } else if (command == "RETR") {
                handleRETR(argument);
            } else if (command == "STOR") {
                handleSTOR(argument);
            } else if (command == "DELE") {
                handleDELE(argument);
            } else if (command == "MKD" || command == "XMKD") {
                handleMKD(argument);
            } else if (command == "RMD" || command == "XRMD") {
                handleRMD(argument);
            } else if (command == "REST") {
                handleREST(argument);
            } else if (command == "APPE") {
                handleAPPE(argument);
            } else if (command == "RNFR") {
                handleRNFR(argument);
            } else if (command == "RNTO") {
                handleRNTO(argument);
            } else {
                sendResponse("502 Command not implemented");
            }
        } else {
            sendResponse("530 Please login with USER and PASS");
        }
    }
    
    active_ = false;
}

void FTPConnection::sendResponse(const std::string& response) {
    if (socket_ < 0) {
        return;
    }
    std::string msg = response;
    if (current_virtual_host_ && response.size() >= 4 && response[3] == ' ') {
        std::string code = response.substr(0, 3);
        std::string custom = current_virtual_host_->getCustomError(code);
        if (!custom.empty()) {
            msg = code + " " + custom;
        }
    }
    std::string full_response = msg + "\r\n";
    ssize_t sent;
    
    if (ssl_active_ && ssl_ && ssl_context_) {
        sent = ssl_context_->writeSSL(ssl_, full_response.c_str(), full_response.length());
    } else {
        sent = send(socket_, full_response.c_str(), full_response.length(), 0);
    }
    
    if (sent < 0) {
        logger_->error("Failed to send response: " + std::string(strerror(errno)));
        active_ = false;
    } else {
        logger_->debug("Sent: " + response);
    }
}

std::string FTPConnection::readLine() {
    if (socket_ < 0) {
        return "";
    }

    std::string line;
    char buffer[1];
    
    while (active_) {
        ssize_t received;
        
        if (ssl_active_ && ssl_ && ssl_context_) {
            received = ssl_context_->readSSL(ssl_, buffer, 1);
        } else {
            received = recv(socket_, buffer, 1, 0);
        }
        
        if (received <= 0) {
            if (received == 0) {
                // Connection closed
                active_ = false;
            }
            break;
        }
        
        if (buffer[0] == '\n') {
            // Remove \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            break;
        }
        
        line += buffer[0];
        
        // Safety limit
        if (line.length() > 1024) {
            break;
        }
    }
    
    return line;
}

// FTP Command Handlers
void FTPConnection::handleUSER(const std::string& username) {
    username_ = username;
    authenticated_ = false;
    sendResponse("331 User name okay, need password");
}

void FTPConnection::handlePASS(const std::string& password) {
    if (username_.empty()) {
        sendResponse("503 Login with USER first");
        return;
    }
    
    bool login_success = false;
    std::shared_ptr<FTPUserManager> auth_mgr = (current_virtual_host_ && current_virtual_host_->getUserManager())
        ? current_virtual_host_->getUserManager() : user_manager_;
    current_user_ = auth_mgr ? auth_mgr->getUser(username_) : nullptr;
    if (current_user_ && current_user_->authenticate(password)) {
        login_success = true;
    } else if (pam_auth_ && pam_auth_->isAvailable()) {
        if (pam_auth_->authenticate(username_, password)) {
            std::string home_directory;
#ifndef _WIN32
            struct passwd* pw = getpwnam(username_.c_str());
            if (pw && pw->pw_dir) {
                home_directory = pw->pw_dir;
            }
#endif
            if (home_directory.empty()) {
                if (current_user_) {
                    home_directory = current_user_->getHomeDirectory();
                } else if (!config_->security.chroot_directory.empty()) {
                    home_directory = config_->security.chroot_directory;
                } else {
                    home_directory = "/tmp";
                }
            }
            
            current_user_ = std::make_shared<FTPUser>(username_, password, home_directory);
            login_success = true;
            logger_->info("PAM authentication successful for user: " + username_);
        } else {
            logger_->warn("PAM authentication failed for user: " + username_);
        }
    }
    
    if (login_success && current_user_) {
        std::string hostname = current_virtual_host_ ? current_virtual_host_->getHostname() : "";
        if (session_tracker_) {
            if (config_->security.max_sessions_per_user > 0 &&
                session_tracker_->getCountForUser(username_) >= static_cast<size_t>(config_->security.max_sessions_per_user)) {
                sendResponse("530 Too many sessions for user");
                return;
            }
            if (current_virtual_host_ && current_virtual_host_->getMaxSessions() > 0 &&
                session_tracker_->getCountForHost(hostname) >= static_cast<size_t>(current_virtual_host_->getMaxSessions())) {
                sendResponse("530 Too many sessions for this host");
                return;
            }
        }
        authenticated_ = true;
        current_directory_ = current_user_->getHomeDirectory();
        if (!isPathWithinHome(current_directory_)) {
            current_directory_ = current_user_->getHomeDirectory();
        }
        if (config_->security.chroot_enabled && !config_->security.chroot_directory.empty()) {
            applyChroot();
        }
        if (session_tracker_) {
            session_tracker_->registerSession(username_, hostname);
            session_registered_ = true;
        }
        sendResponse("230 User logged in, proceed");
        logger_->info("User " + username_ + " logged in");
    } else {
        sendResponse("530 Login incorrect");
        logger_->warn("Failed login attempt for user: " + username_);
    }
}

void FTPConnection::handleQUIT() {
    sendResponse("221 Goodbye");
    active_ = false;
}

void FTPConnection::handleHOST(const std::string& hostname) {
    if (hostname.empty()) {
        sendResponse("501 HOST requires hostname");
        return;
    }
    if (!virtual_host_manager_) {
        sendResponse("502 Virtual hosting not configured");
        return;
    }
    auto host = virtual_host_manager_->getVirtualHost(hostname);
    if (!host || !host->isEnabled()) {
        sendResponse("550 Virtual host not found or disabled: " + hostname);
        return;
    }
    current_virtual_host_ = host;
    logger_->info("Virtual host selected: " + hostname);
    sendResponse("220 Virtual host accepted: " + hostname);
}

void FTPConnection::handlePWD() {
    sendResponse("257 \"" + current_directory_ + "\"");
}

void FTPConnection::handleCWD(const std::string& path) {
    std::string new_path = resolvePath(path);
    
    if (!validatePath(new_path)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (std::filesystem::exists(new_path) && std::filesystem::is_directory(new_path)) {
        current_directory_ = new_path;
        sendResponse("250 CWD command successful");
    } else {
        sendResponse("550 Failed to change directory");
    }
}

void FTPConnection::handleLIST(const std::string& path) {
    if (!hasPermission("list", "")) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string list_path = path.empty() ? current_directory_ : resolvePath(path);
    
    if (!validatePath(list_path)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (!std::filesystem::exists(list_path)) {
        sendResponse("550 File or directory not found");
        return;
    }
    
    sendResponse("150 Opening ASCII mode data connection for file list");
    
    // Accept data connection
    int data_fd = acceptDataConnection();
    if (data_fd < 0) {
        sendResponse("425 Can't open data connection");
        return;
    }
    
    std::string listing;
    if (std::filesystem::is_directory(list_path)) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(list_path)) {
                std::string filename = entry.path().filename().string();
                auto status = entry.status();
                std::string perms = std::filesystem::is_directory(status) ? "d" : "-";
                perms += "rw-rw-rw-";
                
                auto size = std::filesystem::is_directory(status) ? 0 : std::filesystem::file_size(entry.path());
                (void)std::filesystem::last_write_time(entry.path()); // Suppress unused warning
                
                listing += perms + " 1 owner group " + std::to_string(size) + " " + filename + "\r\n";
            }
        } catch (const std::exception& e) {
            logger_->error("Error listing directory: " + std::string(e.what()));
            close(data_fd);
            sendResponse("550 Error listing directory");
            return;
        }
    } else {
        // Single file
        listing += "-rw-rw-rw- 1 owner group " + 
                  std::to_string(std::filesystem::file_size(list_path)) + " " +
                  std::filesystem::path(list_path).filename().string() + "\r\n";
    }
    
    // Send listing through data connection
    send(data_fd, listing.c_str(), listing.length(), 0);
    close(data_fd);
    sendResponse("226 Transfer complete");
}

void FTPConnection::handlePASV() {
    // Disable active mode if it was enabled
    active_mode_enabled_ = false;
    closeDataSocket(); // Close any existing passive socket
    
    int port = createPassiveDataSocket();
    if (port < 0) {
        sendResponse("425 Can't open passive connection");
        return;
    }
    
    std::string response = formatPassiveResponse(port);
    sendResponse(response);
    logger_->debug("Passive mode enabled on port " + std::to_string(port));
}

void FTPConnection::handlePORT(const std::string& address_port) {
    // Parse PORT command: PORT h1,h2,h3,h4,p1,p2
    // Example: PORT 192,168,1,100,4,28
    std::vector<int> parts;
    std::istringstream iss(address_port);
    std::string token;
    
    while (std::getline(iss, token, ',')) {
        try {
            parts.push_back(std::stoi(token));
        } catch (...) {
            sendResponse("501 Invalid PORT command format");
            return;
        }
    }
    
    if (parts.size() != 6) {
        sendResponse("501 Invalid PORT command format");
        return;
    }
    
    // Build IP address
    active_mode_ip_ = std::to_string(parts[0]) + "." +
                      std::to_string(parts[1]) + "." +
                      std::to_string(parts[2]) + "." +
                      std::to_string(parts[3]);
    
    // Calculate port: p1 * 256 + p2
    active_mode_port_ = parts[4] * 256 + parts[5];
    
    if (active_mode_port_ < 1024 || active_mode_port_ > 65535) {
        sendResponse("501 Invalid port number");
        return;
    }
    
    // Close any existing passive connection
    closeDataSocket();
    active_mode_enabled_ = true;
    
    logger_->info("Active mode enabled: " + active_mode_ip_ + ":" + std::to_string(active_mode_port_));
    sendResponse("200 PORT command successful");
}

void FTPConnection::handleTYPE(const std::string& type) {
    if (type == "A" || type == "I") {
        transfer_type_ = type;
        sendResponse("200 Type set to " + type);
    } else {
        sendResponse("504 Command not implemented for that parameter");
    }
}

void FTPConnection::handleSIZE(const std::string& filename) {
    std::string filepath = resolvePath(filename);
    
    if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath)) {
        auto size = std::filesystem::file_size(filepath);
        sendResponse("213 " + std::to_string(size));
    } else {
        sendResponse("550 File not found");
    }
}

void FTPConnection::handleRETR(const std::string& filename) {
    if (!hasPermission("read", filename)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (!std::filesystem::exists(filepath) || !std::filesystem::is_regular_file(filepath)) {
        sendResponse("550 File not found");
        return;
    }
    
    sendResponse("150 Opening " + transfer_type_ + " mode data connection");
    
    int data_fd = acceptDataConnection();
    if (data_fd < 0) {
        sendResponse("425 Can't open data connection");
        return;
    }
    
    size_t total_bytes = 0;
    const size_t buf_size = config_->transfer.buffer_size > 0 ? config_->transfer.buffer_size : 32768;
    const int max_rate = config_->rate_limit.max_transfer_rate;
    auto start_time = std::chrono::steady_clock::now();
    bool use_sendfile = config_->transfer.use_sendfile;
    bool use_mmap = config_->transfer.use_mmap;

#if defined(_WIN32)
    use_sendfile = false;
    use_mmap = false;
#endif

    int file_fd = open(filepath.c_str(), O_RDONLY);
    if (file_fd < 0) {
        close(data_fd);
        sendResponse("550 Failed to open file");
        return;
    }
    struct stat st;
    if (fstat(file_fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(file_fd);
        close(data_fd);
        sendResponse("550 Invalid file");
        return;
    }
    off_t file_size = st.st_size;
    off_t offset = static_cast<off_t>(resume_position_);
    if (offset > 0) {
        logger_->debug("Resuming transfer from position: " + std::to_string(resume_position_));
        if (offset >= file_size) {
            close(file_fd);
            close(data_fd);
            resume_position_ = 0;
            sendResponse("226 Transfer complete");
            return;
        }
    }
    off_t remaining = file_size - offset;

    auto apply_throttle = [&](size_t bytes_so_far) {
        if (max_rate <= 0) return;
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        if (elapsed_ms > 0) {
            size_t allowed = static_cast<size_t>(max_rate * elapsed_ms) / 1000;
            if (bytes_so_far > allowed) {
                size_t delay_ms = ((bytes_so_far - allowed) * 1000) / static_cast<size_t>(max_rate);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    };

    bool transfer_ok = false;
#if defined(__linux__)
    if (use_sendfile && remaining > 0) {
        while (remaining > 0) {
            apply_throttle(total_bytes);
            size_t chunk = static_cast<size_t>(std::min(static_cast<off_t>(buf_size), remaining));
            ssize_t n = sendfile(data_fd, file_fd, &offset, chunk);
            if (n < 0) {
                logger_->error("sendfile failed: " + std::string(strerror(errno)));
                break;
            }
            if (n == 0) break;
            total_bytes += n;
            remaining -= n;
            if (n < static_cast<ssize_t>(chunk)) break;
        }
        transfer_ok = (remaining == 0);
    }
#elif defined(__APPLE__)
    if (use_sendfile && remaining > 0) {
        while (remaining > 0) {
            apply_throttle(total_bytes);
            off_t len = std::min(static_cast<off_t>(buf_size), remaining);
            off_t sent = len;
            int r = sendfile(file_fd, data_fd, offset, &sent, nullptr, 0);
            if (r != 0 || sent <= 0) break;
            total_bytes += sent;
            offset += sent;
            remaining -= sent;
            if (sent < len) break;
        }
        transfer_ok = (remaining == 0);
    }
#endif

#if !defined(_WIN32)
    if (!transfer_ok && use_mmap && file_size > 0 && offset < file_size) {
        size_t map_len = static_cast<size_t>(file_size);
        void* mapped = mmap(nullptr, map_len, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (mapped != MAP_FAILED) {
            const char* p = static_cast<const char*>(mapped) + offset;
            remaining = file_size - offset;
            while (remaining > 0) {
                apply_throttle(total_bytes);
                size_t chunk = std::min(buf_size, static_cast<size_t>(remaining));
                ssize_t sent = send(data_fd, p, chunk, 0);
                if (sent <= 0) break;
                total_bytes += sent;
                p += sent;
                remaining -= sent;
            }
            munmap(mapped, map_len);
            transfer_ok = (remaining == 0);
        }
    }
#endif

    if (!transfer_ok) {
        // Fallback: read/send loop
        if (lseek(file_fd, offset, SEEK_SET) != static_cast<off_t>(offset)) {
            close(file_fd);
            close(data_fd);
            sendResponse("550 Seek failed");
            return;
        }
        std::vector<char> buffer(buf_size);
        remaining = file_size - offset;
        while (remaining > 0) {
            apply_throttle(total_bytes);
            size_t to_read = std::min(buf_size, static_cast<size_t>(remaining));
            ssize_t n = read(file_fd, buffer.data(), to_read);
            if (n <= 0) break;
            ssize_t sent = send(data_fd, buffer.data(), static_cast<size_t>(n), 0);
            if (sent <= 0) {
                logger_->error("Error sending file data: " + std::string(strerror(errno)));
                break;
            }
            total_bytes += sent;
            remaining -= sent;
            if (sent < n) break;
        }
        transfer_ok = (remaining == 0);
    }

    if (!transfer_ok && total_bytes > 0) {
        logger_->warn("Transfer incomplete: " + filename + " (" + std::to_string(total_bytes) + " bytes sent)");
    }

    close(file_fd);
    close(data_fd);
    resume_position_ = 0;
    logger_->info("File transfer complete: " + filename + " (" + std::to_string(total_bytes) + " bytes)");
    sendResponse("226 Transfer complete");
}

void FTPConnection::handleSTOR(const std::string& filename) {
    if (!hasPermission("write", filename)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }
    // Storage quota (v0.3.0)
    uint64_t quota = 0;
    if (current_user_ && current_user_->getStorageQuotaBytes() > 0) {
        quota = current_user_->getStorageQuotaBytes();
    }
    if (current_virtual_host_ && current_virtual_host_->getStorageQuotaBytes() > 0) {
        uint64_t host_quota = current_virtual_host_->getStorageQuotaBytes();
        quota = (quota == 0) ? host_quota : std::min(quota, host_quota);
    }
    if (quota > 0 && current_user_) {
        uint64_t current_size = getDirectorySize(current_user_->getHomeDirectory());
        if (current_size >= quota) {
            sendResponse("552 Storage quota exceeded");
            return;
        }
    }
    sendResponse("150 Opening " + transfer_type_ + " mode data connection");
    
    // Accept data connection
    int data_fd = acceptDataConnection();
    if (data_fd < 0) {
        sendResponse("425 Can't open data connection");
        return;
    }
    
    // Create parent directory if needed
    std::filesystem::path file_path(filepath);
    if (file_path.has_parent_path()) {
        std::filesystem::create_directories(file_path.parent_path());
    }
    
    // Open file for writing (with resume support)
    std::ofstream file;
    if (resume_position_ > 0 && std::filesystem::exists(filepath)) {
        // Resume transfer - open in append mode from resume position
        file.open(filepath, std::ios::binary | std::ios::in | std::ios::out);
        if (file.is_open()) {
            file.seekp(resume_position_);
            logger_->debug("Resuming upload from position: " + std::to_string(resume_position_));
        }
    }
    
    if (!file.is_open()) {
        file.open(filepath, std::ios::binary | std::ios::out);
    }
    
    if (!file.is_open()) {
        close(data_fd);
        sendResponse("550 Failed to create file");
        return;
    }
    
    // Receive file with bandwidth throttling
    char buffer[8192];
    size_t total_bytes = 0;
    auto start_time = std::chrono::steady_clock::now();
    int max_rate = config_->rate_limit.max_transfer_rate;
    
    while (true) {
        ssize_t received = recv(data_fd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break; // Connection closed or error
        }
        
        // Bandwidth throttling for uploads
        if (max_rate > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            if (elapsed > 0) {
                size_t allowed_bytes = (max_rate * elapsed) / 1000;
                if (total_bytes + received > allowed_bytes) {
                    size_t delay_ms = ((total_bytes + received - allowed_bytes) * 1000) / max_rate;
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                }
            }
        }
        
        file.write(buffer, received);
        total_bytes += received;
    }
    
    file.close();
    close(data_fd);
    resume_position_ = 0; // Reset resume position after transfer
    logger_->info("File upload complete: " + filename + " (" + std::to_string(total_bytes) + " bytes)");
    logger_->info("[AUDIT] FILE_UPLOAD user=" + username_ + " file=" + filename + " size=" + std::to_string(total_bytes));
    sendResponse("226 Transfer complete");
}

void FTPConnection::handleDELE(const std::string& filename) {
    if (!hasPermission("write", filename)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath)) {
        if (std::filesystem::remove(filepath)) {
            sendResponse("250 DELE command successful");
        } else {
            sendResponse("550 Failed to delete file");
        }
    } else {
        sendResponse("550 File not found");
    }
}

void FTPConnection::handleMKD(const std::string& dirname) {
    if (!hasPermission("write", dirname)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string dirpath = resolvePath(dirname);
    
    if (!validatePath(dirpath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (std::filesystem::create_directory(dirpath)) {
        sendResponse("257 \"" + dirpath + "\" created");
        logger_->info("[AUDIT] DIR_CREATE user=" + username_ + " dir=" + dirname);
    } else {
        sendResponse("550 Failed to create directory");
    }
}

void FTPConnection::handleRMD(const std::string& dirname) {
    if (!hasPermission("write", dirname)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string dirpath = resolvePath(dirname);
    
    if (!validatePath(dirpath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (std::filesystem::exists(dirpath) && std::filesystem::is_directory(dirpath)) {
        if (std::filesystem::remove(dirpath)) {
            sendResponse("250 RMD command successful");
            logger_->info("[AUDIT] DIR_DELETE user=" + username_ + " dir=" + dirname);
        } else {
            sendResponse("550 Failed to remove directory");
        }
    } else {
        sendResponse("550 Directory not found");
    }
}

std::string FTPConnection::resolvePath(const std::string& path) {
    if (path.empty()) {
        return current_directory_;
    }
    
    std::string resolved;
    if (path[0] == '/') {
        // Absolute path - resolve relative to user's home directory
        if (current_user_) {
            resolved = current_user_->getHomeDirectory() + path;
        } else {
            resolved = path;
        }
    } else {
        // Relative path
        resolved = current_directory_ + "/" + path;
    }
    
    // Normalize path (remove .. and .)
    std::filesystem::path p(resolved);
    try {
        return std::filesystem::canonical(p).string();
    } catch (const std::exception&) {
        // If canonical fails, at least normalize
        return p.lexically_normal().string();
    }
}

bool FTPConnection::validatePath(const std::string& path) {
    if (!current_user_) {
        return false;
    }
    
    std::string resolved = resolvePath(path);
    
    // Check if resolved path is within home directory
    if (!isPathWithinHome(resolved)) {
        return false;
    }
    
    // When virtual host is set, also constrain to host root
    if (current_virtual_host_) {
        std::string root = current_virtual_host_->getRootDirectory();
        if (!root.empty()) {
            std::filesystem::path res_p(resolved), root_p(root);
            try {
                std::filesystem::path canonical_res = std::filesystem::canonical(res_p);
                std::filesystem::path canonical_root = std::filesystem::canonical(root_p);
                auto res_it = canonical_res.begin();
                auto root_it = canonical_root.begin();
                for (; root_it != canonical_root.end(); ++root_it, ++res_it) {
                    if (res_it == canonical_res.end() || *res_it != *root_it) {
                        return false;
                    }
                }
            } catch (const std::exception&) {
                return false;
            }
        }
    }
    return true;
}

bool FTPConnection::isPathWithinHome(const std::string& path) {
    if (!current_user_) {
        return false;
    }
    
    std::string home = current_user_->getHomeDirectory();
    std::filesystem::path path_p(path);
    std::filesystem::path home_p(home);
    
    try {
        std::filesystem::path canonical_path = std::filesystem::canonical(path_p);
        std::filesystem::path canonical_home = std::filesystem::canonical(home_p);
        
        // Check if canonical_path starts with canonical_home
        auto it = canonical_path.begin();
        auto home_it = canonical_home.begin();
        
        while (home_it != canonical_home.end()) {
            if (it == canonical_path.end() || *it != *home_it) {
                return false;
            }
            ++it;
            ++home_it;
        }
        return true;
    } catch (const std::exception&) {
        // If canonical fails, do simple string comparison
        return path.find(home) == 0;
    }
}

bool FTPConnection::hasPermission(const std::string& operation, const std::string& path) {
    if (!current_user_) {
        return false;
    }
    
    // Basic permission check - delegate to FTPUser
    return current_user_->hasPermission(operation, path);
}

int FTPConnection::createPassiveDataSocket() {
    closeDataSocket();
    
    // Create socket
    passive_listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (passive_listen_socket_ < 0) {
        logger_->error("Failed to create passive socket: " + std::string(strerror(errno)));
        return -1;
    }
    
    // Set socket options
    int reuse = 1;
    setsockopt(passive_listen_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind to any available port in the range
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    int port_range_start = config_->connection.passive_port_range_start;
    int port_range_end = config_->connection.passive_port_range_end;
    
    for (int port = port_range_start; port <= port_range_end; ++port) {
        addr.sin_port = htons(port);
        if (bind(passive_listen_socket_, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            if (listen(passive_listen_socket_, 1) == 0) {
                logger_->debug("Passive socket listening on port " + std::to_string(port));
                return port;
            }
        }
    }
    
    close(passive_listen_socket_);
    passive_listen_socket_ = -1;
    logger_->error("Failed to bind passive socket in port range");
    return -1;
}

int FTPConnection::acceptDataConnection() {
    std::lock_guard<std::mutex> lock(data_socket_mutex_);
    
    if (active_mode_enabled_) {
        return connectActiveDataSocket();
    }
    
    if (passive_listen_socket_ < 0) {
        logger_->error("No passive socket available");
        return -1;
    }
    
    // Set socket to non-blocking for timeout
    int flags = fcntl(passive_listen_socket_, F_GETFL, 0);
    fcntl(passive_listen_socket_, F_SETFL, flags | O_NONBLOCK);
    
    // Wait for connection with timeout
    struct timeval timeout;
    timeout.tv_sec = 10; // 10 second timeout
    timeout.tv_usec = 0;
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(passive_listen_socket_, &read_fds);
    
    int result = select(passive_listen_socket_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (result <= 0) {
        logger_->error("Timeout waiting for data connection");
        return -1;
    }
    
    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    data_socket_ = accept(passive_listen_socket_, (struct sockaddr*)&client_addr, &client_len);
    
    if (data_socket_ < 0) {
        logger_->error("Failed to accept data connection: " + std::string(strerror(errno)));
        return -1;
    }
    
    // Restore blocking mode
    fcntl(passive_listen_socket_, F_SETFL, flags);
    
    logger_->debug("Data connection accepted from " + std::string(inet_ntoa(client_addr.sin_addr)));
    return data_socket_;
}

int FTPConnection::connectActiveDataSocket() {
    if (active_mode_ip_.empty() || active_mode_port_ <= 0) {
        logger_->error("Active mode parameters not set");
        return -1;
    }
    
    data_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket_ < 0) {
        logger_->error("Failed to create active mode socket: " + std::string(strerror(errno)));
        return -1;
    }
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(active_mode_port_);
    
    if (inet_pton(AF_INET, active_mode_ip_.c_str(), &addr.sin_addr) <= 0) {
        logger_->error("Invalid active mode IP: " + active_mode_ip_);
        close(data_socket_);
        data_socket_ = -1;
        active_mode_enabled_ = false;
        active_mode_ip_.clear();
        active_mode_port_ = 0;
        return -1;
    }
    
    // Set connect timeout (10 seconds)
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(data_socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(data_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    if (connect(data_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        logger_->error("Failed to connect to active mode target " + active_mode_ip_ + ":" +
                       std::to_string(active_mode_port_) + " - " + std::string(strerror(errno)));
        close(data_socket_);
        data_socket_ = -1;
        active_mode_enabled_ = false;
        active_mode_ip_.clear();
        active_mode_port_ = 0;
        return -1;
    }
    
    logger_->debug("Active mode data connection established to " + active_mode_ip_ + ":" +
                   std::to_string(active_mode_port_));
    active_mode_enabled_ = false;
    active_mode_ip_.clear();
    active_mode_port_ = 0;
    return data_socket_;
}

void FTPConnection::closeDataSocket() {
    std::lock_guard<std::mutex> lock(data_socket_mutex_);
    
    if (data_socket_ >= 0) {
        close(data_socket_);
        data_socket_ = -1;
    }
    
    if (passive_listen_socket_ >= 0) {
        close(passive_listen_socket_);
        passive_listen_socket_ = -1;
    }
}

std::string FTPConnection::formatPassiveResponse(int port) {
    // Get server IP (simplified - use localhost for now)
    // In production, should get actual server IP
    std::string ip = "127,0,0,1";
    
    int p1 = port / 256;
    int p2 = port % 256;
    
    return "227 Entering Passive Mode (" + ip + "," + std::to_string(p1) + "," + std::to_string(p2) + ")";
}

// SSL/TLS Command Handlers
void FTPConnection::handleAUTH(const std::string& method) {
    std::string method_upper = method;
    std::transform(method_upper.begin(), method_upper.end(), method_upper.begin(), ::toupper);
    
    if (method_upper == "TLS" || method_upper == "SSL") {
        if (ssl_active_) {
            sendResponse("534 SSL/TLS already active");
            return;
        }
        // Per-host SSL (v0.3.0): use virtual host cert if set
        if (current_virtual_host_ && !current_virtual_host_->getSslCertFile().empty() &&
            !current_virtual_host_->getSslKeyFile().empty()) {
            if (!ssl_context_) {
                ssl_context_ = std::make_shared<SSLContext>(logger_);
            }
            if (ssl_context_->initialize(current_virtual_host_->getSslCertFile(),
                    current_virtual_host_->getSslKeyFile(), current_virtual_host_->getSslCaFile(), false, "")) {
                ssl_enabled_ = true;
            }
        }
        if (!ssl_enabled_ || !ssl_context_) {
            sendResponse("534 SSL/TLS not available");
            return;
        }
        sendResponse("234 AUTH TLS successful");
        if (!upgradeToSSL()) {
            logger_->error("Failed to upgrade connection to SSL");
            active_ = false;
        } else {
            ssl_active_ = true;
            logger_->info("Connection upgraded to SSL/TLS");
        }
    } else {
        sendResponse("504 Unsupported AUTH method");
    }
}

void FTPConnection::handlePBSZ(const std::string& size) {
    (void)size; // PBSZ size parameter is ignored for TLS
    if (!ssl_active_) {
        sendResponse("503 PBSZ command only valid in secure mode");
        return;
    }
    
    // PBSZ (Protection Buffer Size) - always 0 for TLS
    sendResponse("200 PBSZ=0");
}

void FTPConnection::handlePROT(const std::string& level) {
    if (!ssl_active_) {
        sendResponse("503 PROT command only valid in secure mode");
        return;
    }
    
    std::string level_upper = level;
    std::transform(level_upper.begin(), level_upper.end(), level_upper.begin(), ::toupper);
    
    if (level_upper == "C" || level_upper == "CLEAR") {
        protection_level_ = "C";
        sendResponse("200 Protection level set to Clear");
    } else if (level_upper == "P" || level_upper == "PRIVATE") {
        protection_level_ = "P";
        sendResponse("200 Protection level set to Private");
    } else if (level_upper == "S" || level_upper == "SAFE") {
        protection_level_ = "S";
        sendResponse("200 Protection level set to Safe");
    } else if (level_upper == "E" || level_upper == "CONFIDENTIAL") {
        protection_level_ = "E";
        sendResponse("200 Protection level set to Confidential");
    } else {
        sendResponse("504 Unsupported protection level");
    }
}

bool FTPConnection::upgradeToSSL() {
    if (!ssl_context_ || !ssl_enabled_) {
        return false;
    }
    
    ssl_ = ssl_context_->createSSL(socket_);
    if (!ssl_) {
        logger_->error("Failed to create SSL connection: " + ssl_context_->getLastError());
        return false;
    }
    
    if (!ssl_context_->acceptSSL(ssl_)) {
        logger_->error("SSL handshake failed: " + ssl_context_->getLastError());
        ssl_context_->freeSSL(ssl_);
        ssl_ = nullptr;
        return false;
    }
    
    return true;
}

void FTPConnection::applyChroot() {
#ifndef _WIN32
    if (config_->security.chroot_enabled && !config_->security.chroot_directory.empty()) {
        std::string chroot_dir = config_->security.chroot_directory;
        
        // Ensure chroot directory exists
        if (!std::filesystem::exists(chroot_dir)) {
            logger_->warn("Chroot directory does not exist: " + chroot_dir);
            return;
        }
        
        // Apply chroot
        if (chroot(chroot_dir.c_str()) != 0) {
            logger_->error("Failed to apply chroot: " + std::string(strerror(errno)));
        } else {
            logger_->info("Chroot applied to: " + chroot_dir);
            // Update current directory to be relative to chroot
            if (current_directory_.find(chroot_dir) == 0) {
                current_directory_ = current_directory_.substr(chroot_dir.length());
                if (current_directory_.empty() || current_directory_[0] != '/') {
                    current_directory_ = "/" + current_directory_;
                }
            } else {
                current_directory_ = "/";
            }
        }
    }
#else
    // Chroot not supported on Windows
    (void)config_;
    logger_->warn("Chroot not supported on Windows");
#endif
}

void FTPConnection::handleREST(const std::string& position) {
    try {
        resume_position_ = std::stoull(position);
        sendResponse("350 Restarting at " + position + ". Send STOR or RETR to initiate transfer");
        logger_->debug("Resume position set to: " + position);
    } catch (...) {
        sendResponse("501 Invalid restart position");
    }
}

void FTPConnection::handleAPPE(const std::string& filename) {
    if (!hasPermission("write", filename)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    sendResponse("150 Opening data connection for append");
    
    int data_fd = acceptDataConnection();
    if (data_fd < 0) {
        sendResponse("425 Can't open data connection");
        return;
    }
    
    std::ofstream file;
    if (std::filesystem::exists(filepath)) {
        file.open(filepath, std::ios::binary | std::ios::app);
    } else {
        file.open(filepath, std::ios::binary | std::ios::out);
    }
    
    if (!file.is_open()) {
        close(data_fd);
        sendResponse("550 Failed to open file for append");
        return;
    }
    
    char buffer[8192];
    ssize_t received;
    while ((received = recv(data_fd, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, received);
    }
    
    file.close();
    close(data_fd);
    resume_position_ = 0; // Reset resume position
    sendResponse("226 Transfer complete");
}

void FTPConnection::handleRNFR(const std::string& filename) {
    if (!hasPermission("write", filename)) {
        sendResponse("550 Permission denied");
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }
    
    if (!std::filesystem::exists(filepath)) {
        sendResponse("550 File or directory not found");
        return;
    }
    
    rename_from_path_ = filepath;
    sendResponse("350 File or directory exists, ready for destination name");
}

void FTPConnection::handleRNTO(const std::string& filename) {
    if (rename_from_path_.empty()) {
        sendResponse("503 RNFR required first");
        return;
    }
    
    if (!hasPermission("write", filename)) {
        sendResponse("550 Permission denied");
        rename_from_path_.clear();
        return;
    }
    
    std::string filepath = resolvePath(filename);
    
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        rename_from_path_.clear();
        return;
    }
    
    if (std::filesystem::exists(filepath)) {
        sendResponse("553 File already exists");
        rename_from_path_.clear();
        return;
    }
    
    try {
        std::filesystem::rename(rename_from_path_, filepath);
        sendResponse("250 Rename successful");
        logger_->info("Renamed: " + rename_from_path_ + " -> " + filepath);
        logger_->info("[AUDIT] FILE_RENAME user=" + username_ + " from=" + rename_from_path_ + " to=" + filepath);
        rename_from_path_.clear();
    } catch (const std::exception& e) {
        sendResponse("550 Rename failed: " + std::string(e.what()));
        rename_from_path_.clear();
    }
}

} // namespace simple_sftpd
