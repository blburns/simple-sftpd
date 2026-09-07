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
#include "simple-sftpd/utils/compression.hpp"
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
#include <ctime>
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

// RFC 3659 timestamps are UTC in YYYYMMDDHHMMSS form.
bool fileModifiedTimeUtc(const std::string& path, std::string& out) {
    struct stat st {};
    if (::stat(path.c_str(), &st) != 0) {
        return false;
    }
    std::time_t mtime = static_cast<std::time_t>(st.st_mtime);
    std::tm tm {};
#ifdef _WIN32
    if (gmtime_s(&tm, &mtime) != 0) {
        return false;
    }
#else
    if (gmtime_r(&mtime, &tm) == nullptr) {
        return false;
    }
#endif
    char buffer[16];
    if (std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &tm) == 0) {
        return false;
    }
    out = buffer;
    return true;
}
} // namespace

FTPConnection::FTPConnection(int socket, std::shared_ptr<Logger> logger, std::shared_ptr<FTPServerConfig> config,
                             std::shared_ptr<FTPVirtualHostManager> vhost_manager,
                             std::shared_ptr<SessionTracker> session_tracker)
    : socket_(socket), logger_(logger), config_(config), user_manager_(),
      active_(false), authenticated_(false), current_user_(nullptr), current_directory_("/"),
      ssl_enabled_(false), ssl_active_(false), ssl_(nullptr), data_ssl_(nullptr),
      passive_listen_socket_(-1), data_socket_(-1), transfer_type_("A"), transfer_mode_("S"),
      protection_level_("C"),
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
        if (client_thread_.get_id() != std::this_thread::get_id()) {
            client_thread_.join();
        } else {
            client_thread_.detach();
        }
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
            sendResponse(" SIZE");
            sendResponse(" REST STREAM");
            sendResponse(" PORT");
            sendResponse(" EPRT");
            sendResponse(" EPSV");
            sendResponse(" MDTM");
            sendResponse(" MLST type*;size*;modify*;perm*;");
            sendResponse(" TVFS");
            sendResponse(" UTF8");
            if (virtual_host_manager_ && !virtual_host_manager_->listVirtualHosts().empty()) {
                sendResponse(" HOST");
            }
            if (ssl_enabled_) {
                sendResponse(" AUTH TLS");
                sendResponse(" PBSZ");
                sendResponse(" PROT");
            }
            if (compressionEnabled()) {
                sendResponse(" MODE Z");
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
        } else if (command == "OPTS") {
            handleOPTS(argument);
        } else if (command == "HELP") {
            handleHELP(argument);
        } else if (command == "ABOR") {
            handleABOR();
        } else if (authenticated_) {
            // Commands that require authentication
            if (command == "PWD" || command == "XPWD") {
                handlePWD();
            } else if (command == "CWD" || command == "XCWD") {
                handleCWD(argument);
            } else if (command == "CDUP" || command == "XCUP") {
                handleCDUP();
            } else if (command == "LIST" || command == "NLST") {
                handleLIST(argument);
            } else if (command == "MLSD") {
                handleMLSD(argument);
            } else if (command == "MLST") {
                handleMLST(argument);
            } else if (command == "MDTM") {
                handleMDTM(argument);
            } else if (command == "STAT") {
                handleSTAT(argument);
            } else if (command == "SITE") {
                handleSITE(argument);
            } else if (command == "ALLO") {
                handleALLO(argument);
            } else if (command == "STOU") {
                handleSTOU(argument);
            } else if (command == "PASV") {
                handlePASV();
            } else if (command == "EPSV") {
                handleEPSV(argument);
            } else if (command == "PORT") {
                handlePORT(argument);
            } else if (command == "EPRT") {
                handleEPRT(argument);
            } else if (command == "MODE") {
                handleMODE(argument);
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
            if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (received == 0) {
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
    if (epsv_all_) {
        sendResponse("501 EPSV ALL is in effect, use EPSV");
        return;
    }

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
    if (epsv_all_) {
        sendResponse("501 EPSV ALL is in effect, use EPSV");
        return;
    }

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

void FTPConnection::handleEPRT(const std::string& endpoint) {
    if (epsv_all_) {
        sendResponse("501 EPSV ALL is in effect, use EPSV");
        return;
    }

    // RFC 2428: EPRT <d><net-prt><d><net-addr><d><tcp-port><d>
    if (endpoint.size() < 7) {
        sendResponse("501 Invalid EPRT command format");
        return;
    }
    const char delim = endpoint[0];
    if (endpoint.back() != delim) {
        sendResponse("501 Invalid EPRT command format");
        return;
    }
    std::vector<std::string> fields;
    std::string token;
    for (size_t i = 1; i < endpoint.size(); ++i) {
        if (endpoint[i] == delim) {
            fields.push_back(token);
            token.clear();
        } else {
            token.push_back(endpoint[i]);
        }
    }
    if (fields.size() != 3) {
        sendResponse("501 Invalid EPRT command format");
        return;
    }
    if (fields[0] != "1") {
        sendResponse("522 Network protocol not supported, use (1)");
        return;
    }
    int port = 0;
    try {
        port = std::stoi(fields[2]);
    } catch (...) {
        sendResponse("501 Invalid EPRT port");
        return;
    }
    if (port < 1024 || port > 65535) {
        sendResponse("501 Invalid port number");
        return;
    }
    closeDataSocket();
    active_mode_ip_ = fields[1];
    active_mode_port_ = port;
    active_mode_enabled_ = true;
    logger_->info("Active mode enabled (EPRT): " + active_mode_ip_ + ":" + std::to_string(active_mode_port_));
    sendResponse("200 EPRT command successful");
}

void FTPConnection::handleCDUP() {
    const std::string parent = resolvePath("..");

    if (!validatePath(parent)) {
        // Refusing here is what keeps CDUP from walking out of the home directory.
        sendResponse("550 Invalid path");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(parent, ec)) {
        sendResponse("550 Failed to change directory");
        return;
    }

    current_directory_ = parent;
    sendResponse("250 CDUP command successful");
}

void FTPConnection::handleEPSV(const std::string& argument) {
    std::string arg = argument;
    std::transform(arg.begin(), arg.end(), arg.begin(), ::toupper);

    if (arg == "ALL") {
        epsv_all_ = true;
        sendResponse("200 EPSV ALL command successful");
        return;
    }

    if (!arg.empty() && arg != "1" && arg != "2") {
        sendResponse("522 Network protocol not supported, use (1)");
        return;
    }

    active_mode_enabled_ = false;
    closeDataSocket();

    const int port = createPassiveDataSocket();
    if (port < 0) {
        sendResponse("425 Can't open passive connection");
        return;
    }

    sendResponse("229 Entering Extended Passive Mode (|||" + std::to_string(port) + "|)");
    logger_->debug("Extended passive mode enabled on port " + std::to_string(port));
}

std::string FTPConnection::buildMachineFacts(const std::string& path, const std::string& name) const {
    std::error_code ec;
    const bool is_directory = std::filesystem::is_directory(path, ec);

    std::string facts = std::string("type=") + (is_directory ? "dir" : "file") + ";";

    if (!is_directory) {
        const auto size = std::filesystem::file_size(path, ec);
        if (!ec) {
            facts += "size=" + std::to_string(size) + ";";
        }
    }

    std::string modify;
    if (fileModifiedTimeUtc(path, modify)) {
        facts += "modify=" + modify + ";";
    }

    const bool writable = current_user_ && current_user_->hasPermission("write", path);
    if (is_directory) {
        facts += std::string("perm=") + (writable ? "elcmpd" : "el") + ";";
    } else {
        facts += std::string("perm=") + (writable ? "adfrw" : "r") + ";";
    }

    return facts + " " + name;
}

void FTPConnection::handleMLSD(const std::string& path) {
    if (!hasPermission("list", path)) {
        sendResponse("550 Permission denied");
        return;
    }

    const std::string target = path.empty() ? current_directory_ : resolvePath(path);
    if (!validatePath(target)) {
        sendResponse("550 Invalid path");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(target, ec)) {
        sendResponse("550 Not a directory");
        return;
    }

    sendResponse("150 Opening data connection for MLSD");

    const int data_fd = acceptDataConnection();
    if (data_fd < 0) {
        sendResponse("425 Can't open data connection");
        return;
    }

    std::string listing;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(target)) {
            listing += buildMachineFacts(entry.path().string(),
                                         entry.path().filename().string()) + "\r\n";
        }
    } catch (const std::exception& e) {
        logger_->error("Error listing directory: " + std::string(e.what()));
        close(data_fd);
        sendResponse("550 Error listing directory");
        return;
    }

    send(data_fd, listing.c_str(), listing.length(), 0);
    close(data_fd);
    sendResponse("226 Transfer complete");
}

void FTPConnection::handleMLST(const std::string& path) {
    const std::string target = path.empty() ? current_directory_ : resolvePath(path);
    if (!validatePath(target)) {
        sendResponse("550 Invalid path");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(target, ec)) {
        sendResponse("550 File or directory not found");
        return;
    }

    const std::string name = path.empty() ? current_directory_ : path;
    sendResponse("250-Listing " + name);
    sendResponse(" " + buildMachineFacts(target, name));
    sendResponse("250 End");
}

void FTPConnection::handleMDTM(const std::string& filename) {
    if (filename.empty()) {
        sendResponse("501 MDTM requires a file name");
        return;
    }

    const std::string filepath = resolvePath(filename);
    if (!validatePath(filepath)) {
        sendResponse("550 Invalid path");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        sendResponse("550 File not found");
        return;
    }

    std::string modify;
    if (!fileModifiedTimeUtc(filepath, modify)) {
        sendResponse("550 Could not determine modification time");
        return;
    }

    sendResponse("213 " + modify);
}

void FTPConnection::handleSTAT(const std::string& argument) {
    if (argument.empty()) {
        sendResponse("211-simple-sftpd status");
        sendResponse(" Logged in as: " + (username_.empty() ? std::string("(none)") : username_));
        sendResponse(" Current directory: " + current_directory_);
        sendResponse(" Transfer type: " + transfer_type_ + ", mode: " + transfer_mode_);
        sendResponse(std::string(" Control connection: ") + (ssl_active_ ? "TLS" : "plaintext"));
        sendResponse(" Data protection: " + protection_level_);
        sendResponse("211 End of status");
        return;
    }

    // STAT <path> reports a listing over the control connection, no data channel.
    const std::string target = resolvePath(argument);
    if (!validatePath(target)) {
        sendResponse("550 Invalid path");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(target, ec)) {
        sendResponse("550 File or directory not found");
        return;
    }

    sendResponse("213-Status of " + argument);
    if (std::filesystem::is_directory(target, ec)) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(target)) {
                sendResponse(" " + buildMachineFacts(entry.path().string(),
                                                     entry.path().filename().string()));
            }
        } catch (const std::exception& e) {
            logger_->error("Error listing directory: " + std::string(e.what()));
        }
    } else {
        sendResponse(" " + buildMachineFacts(target,
                                             std::filesystem::path(target).filename().string()));
    }
    sendResponse("213 End of status");
}

void FTPConnection::handleOPTS(const std::string& argument) {
    std::istringstream iss(argument);
    std::string option;
    std::string value;
    iss >> option >> value;
    std::transform(option.begin(), option.end(), option.begin(), ::toupper);
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);

    if (option == "UTF8") {
        if (value.empty() || value == "ON") {
            utf8_enabled_ = true;
            sendResponse("200 UTF8 set to on");
        } else if (value == "OFF") {
            utf8_enabled_ = false;
            sendResponse("200 UTF8 set to off");
        } else {
            sendResponse("501 Invalid UTF8 option");
        }
        return;
    }

    if (option == "MLST") {
        sendResponse("200 MLST OPTS type;size;modify;perm;");
        return;
    }

    sendResponse("501 Option not supported");
}

void FTPConnection::handleABOR() {
    // Transfers run synchronously on this connection's thread, so an ABOR that
    // reaches the dispatcher means nothing is in flight.
    closeDataSocket();
    sendResponse("226 ABOR command successful");
}

void FTPConnection::handleSTOU(const std::string& filename) {
    const std::string base = filename.empty()
        ? std::string("ftpd")
        : std::filesystem::path(filename).filename().string();

    std::string unique;
    for (int attempt = 0; attempt < 10000; ++attempt) {
        const std::string candidate = base + "." + std::to_string(attempt);
        std::error_code ec;
        if (!std::filesystem::exists(current_directory_ + "/" + candidate, ec)) {
            unique = candidate;
            break;
        }
    }

    if (unique.empty()) {
        sendResponse("452 Could not allocate a unique file name");
        return;
    }

    // RFC 1123 requires the 150 reply to carry the generated name; STOR reads this.
    store_unique_name_ = unique;
    resume_position_ = 0;
    handleSTOR(unique);
    store_unique_name_.clear();
}

void FTPConnection::handleSITE(const std::string& argument) {
    std::istringstream iss(argument);
    std::string subcommand;
    iss >> subcommand;
    std::transform(subcommand.begin(), subcommand.end(), subcommand.begin(), ::toupper);

    if (subcommand.empty() || subcommand == "HELP") {
        sendResponse("214-The following SITE commands are recognized:");
        sendResponse(" CHMOD <mode> <path>");
        sendResponse(" UMASK");
        sendResponse("214 SITE help complete");
        return;
    }

    if (subcommand == "UMASK") {
        sendResponse("200 UMASK is fixed at 022");
        return;
    }

    if (subcommand == "CHMOD") {
        std::string mode_text;
        std::string target;
        iss >> mode_text;
        std::getline(iss, target);
        target.erase(0, target.find_first_not_of(" \t"));

        if (mode_text.empty() || target.empty()) {
            sendResponse("501 Usage: SITE CHMOD <mode> <path>");
            return;
        }
        if (!hasPermission("write", target)) {
            sendResponse("550 Permission denied");
            return;
        }

        const std::string path = resolvePath(target);
        if (!validatePath(path)) {
            sendResponse("550 Invalid path");
            return;
        }

        unsigned long mode = 0;
        try {
            mode = std::stoul(mode_text, nullptr, 8);
        } catch (const std::exception&) {
            sendResponse("501 Invalid mode");
            return;
        }

        std::error_code ec;
        std::filesystem::permissions(path,
                                     static_cast<std::filesystem::perms>(mode & 07777),
                                     std::filesystem::perm_options::replace, ec);
        if (ec) {
            sendResponse("550 SITE CHMOD failed");
            return;
        }
        sendResponse("200 SITE CHMOD command successful");
        return;
    }

    sendResponse("500 Unknown SITE command");
}

void FTPConnection::handleALLO(const std::string& argument) {
    (void)argument;
    sendResponse("202 ALLO not required, storage is allocated on demand");
}

void FTPConnection::handleHELP(const std::string& argument) {
    (void)argument;
    sendResponse("214-The following commands are recognized:");
    sendResponse(" USER PASS QUIT NOOP SYST FEAT OPTS HELP HOST");
    sendResponse(" AUTH PBSZ PROT");
    sendResponse(" PWD XPWD CWD XCWD CDUP XCUP");
    sendResponse(" LIST NLST MLSD MLST STAT SIZE MDTM");
    sendResponse(" PASV EPSV PORT EPRT MODE TYPE ALLO ABOR");
    sendResponse(" RETR STOR STOU APPE REST DELE");
    sendResponse(" MKD XMKD RMD XRMD RNFR RNTO SITE");
    sendResponse("214 Help command successful");
}

bool FTPConnection::compressionEnabled() const {
#ifdef ENABLE_COMPRESSION
    return config_ && config_->transfer.enable_compression;
#else
    return false;
#endif
}

void FTPConnection::handleMODE(const std::string& mode) {
    std::string m = mode;
    std::transform(m.begin(), m.end(), m.begin(), ::toupper);
    if (m == "S" || m == "STREAM") {
        transfer_mode_ = "S";
        sendResponse("200 Mode set to Stream");
        return;
    }
    if (m == "Z") {
        if (!compressionEnabled()) {
            sendResponse("504 MODE Z is not enabled");
            return;
        }
        transfer_mode_ = "Z";
        sendResponse("200 Mode set to Compressed (Z)");
        return;
    }
    sendResponse("504 Command not implemented for that parameter");
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

    if (transferModeZ()) {
        if (resume_position_ > 0) {
            sendResponse("550 REST is not supported with MODE Z");
            resume_position_ = 0;
            return;
        }
        if (!compressionEnabled()) {
            sendResponse("504 MODE Z is not enabled");
            return;
        }
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
    const bool mode_z = transferModeZ();
    if (mode_z) {
        use_sendfile = false;
        use_mmap = false;
    }

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
    if (mode_z) {
        if (lseek(file_fd, offset, SEEK_SET) != static_cast<off_t>(offset)) {
            close(file_fd);
            close(data_fd);
            sendResponse("550 Seek failed");
            return;
        }
        ZlibStream stream(ZlibStream::Mode::Deflate);
        if (!stream.valid()) {
            close(file_fd);
            close(data_fd);
            sendResponse("426 Compression is not available");
            return;
        }
        std::vector<char> buffer(buf_size);
        remaining = file_size - offset;
        bool ok = true;
        while (remaining > 0) {
            apply_throttle(total_bytes);
            size_t to_read = std::min(buf_size, static_cast<size_t>(remaining));
            ssize_t n = read(file_fd, buffer.data(), to_read);
            if (n <= 0) {
                ok = false;
                break;
            }
            std::vector<uint8_t> compressed;
            if (!stream.process(reinterpret_cast<const uint8_t*>(buffer.data()), static_cast<size_t>(n),
                                compressed, false)) {
                ok = false;
                break;
            }
            if (!compressed.empty()) {
                ssize_t sent = send(data_fd, compressed.data(), compressed.size(), 0);
                if (sent <= 0 || static_cast<size_t>(sent) != compressed.size()) {
                    ok = false;
                    break;
                }
                total_bytes += static_cast<size_t>(sent);
            }
            remaining -= n;
        }
        if (ok) {
            std::vector<uint8_t> compressed;
            ok = stream.process(nullptr, 0, compressed, true);
            if (ok && !compressed.empty()) {
                ssize_t sent = send(data_fd, compressed.data(), compressed.size(), 0);
                ok = (sent > 0 && static_cast<size_t>(sent) == compressed.size());
                if (ok) {
                    total_bytes += static_cast<size_t>(sent);
                }
            }
        }
        transfer_ok = ok;
    }
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

    if (!transfer_ok && !mode_z) {
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

    if (transferModeZ()) {
        if (resume_position_ > 0) {
            sendResponse("550 REST is not supported with MODE Z");
            resume_position_ = 0;
            return;
        }
        if (!compressionEnabled()) {
            sendResponse("504 MODE Z is not enabled");
            return;
        }
    }

    if (store_unique_name_.empty()) {
        sendResponse("150 Opening " + transfer_type_ + " mode data connection");
    } else {
        sendResponse("150 FILE: " + store_unique_name_);
    }
    
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
    bool upload_ok = true;

    if (transferModeZ()) {
        ZlibStream stream(ZlibStream::Mode::Inflate);
        if (!stream.valid()) {
            file.close();
            close(data_fd);
            sendResponse("426 Compression is not available");
            return;
        }
        while (true) {
            ssize_t received = recv(data_fd, buffer, sizeof(buffer), 0);
            if (received < 0) {
                upload_ok = false;
                break;
            }
            std::vector<uint8_t> plain;
            if (!stream.process(reinterpret_cast<const uint8_t*>(buffer),
                                received > 0 ? static_cast<size_t>(received) : 0,
                                plain, received == 0)) {
                upload_ok = false;
                break;
            }
            if (!plain.empty()) {
                file.write(reinterpret_cast<const char*>(plain.data()),
                           static_cast<std::streamsize>(plain.size()));
                total_bytes += plain.size();
            }
            if (received == 0) {
                break;
            }
            if (max_rate > 0) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                if (elapsed > 0) {
                    size_t allowed_bytes = (max_rate * elapsed) / 1000;
                    if (total_bytes > allowed_bytes) {
                        size_t delay_ms = ((total_bytes - allowed_bytes) * 1000) / max_rate;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                }
            }
        }
    } else {
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
    }

    file.close();
    close(data_fd);
    resume_position_ = 0; // Reset resume position after transfer
    if (!upload_ok) {
        logger_->error("Compressed upload failed: " + filename);
        sendResponse("426 Transfer aborted");
        return;
    }
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

namespace {

// weakly_canonical resolves symlinks and ".." for paths whose trailing
// components do not exist yet, which is what STOR/MKD targets look like.
std::filesystem::path normalizeForComparison(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec || normalized.empty()) {
        normalized = path.lexically_normal();
    }
    return normalized;
}

// True when `child` is `parent` or lives underneath it. Compares whole path
// components so that "/srv/ftproot-evil" is not treated as under "/srv/ftproot".
bool isWithin(const std::filesystem::path& child, const std::filesystem::path& parent) {
    auto parent_it = parent.begin();
    auto child_it = child.begin();
    for (; parent_it != parent.end(); ++parent_it, ++child_it) {
        if (child_it == child.end() || *child_it != *parent_it) {
            return false;
        }
    }
    return true;
}

} // namespace

std::string FTPConnection::resolvePath(const std::string& path) {
    if (path.empty()) {
        return current_directory_;
    }

    std::filesystem::path candidate;
    if (path[0] == '/') {
        // Absolute paths are virtual: the user's home directory is their root.
        const std::string home = current_user_ ? current_user_->getHomeDirectory() : std::string("/");
        candidate = std::filesystem::path(home) / std::filesystem::path(path).relative_path();
    } else {
        candidate = std::filesystem::path(current_directory_) / path;
    }

    return normalizeForComparison(candidate).string();
}

// Expects a path already run through resolvePath(); re-resolving here would
// re-apply the home prefix to an absolute real path and defeat the check.
bool FTPConnection::validatePath(const std::string& path) {
    if (!current_user_) {
        return false;
    }

    if (!isPathWithinHome(path)) {
        return false;
    }

    // When a virtual host is selected, also constrain to its root.
    if (current_virtual_host_) {
        const std::string root = current_virtual_host_->getRootDirectory();
        if (!root.empty() && !isWithin(normalizeForComparison(path), normalizeForComparison(root))) {
            return false;
        }
    }

    return true;
}

bool FTPConnection::isPathWithinHome(const std::string& path) {
    if (!current_user_) {
        return false;
    }

    return isWithin(normalizeForComparison(path),
                    normalizeForComparison(current_user_->getHomeDirectory()));
}

bool FTPConnection::hasPermission(const std::string& operation, const std::string& path) {
    if (!current_user_) {
        return false;
    }

    // Resolve first so path-scoped grants are matched against a real path
    // rather than whatever spelling the client happened to send.
    const std::string resolved = path.empty() ? current_directory_ : resolvePath(path);
    return current_user_->hasPermission(operation, resolved);
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
    if (transferModeZ()) {
        sendResponse("550 REST is not supported with MODE Z");
        return;
    }
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

    if (transferModeZ()) {
        sendResponse("550 APPE is not supported with MODE Z");
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
