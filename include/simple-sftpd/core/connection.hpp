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

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace simple_sftpd {

class Logger;
class FTPServerConfig;
class FTPUserManager;
class FTPUser;
class SSLContext;
class FileCache;
class PAMAuth;
class FTPVirtualHostManager;
class FTPVirtualHost;
class SessionTracker;

class FTPConnection {
public:
    FTPConnection(int socket, std::shared_ptr<Logger> logger, std::shared_ptr<FTPServerConfig> config,
                  std::shared_ptr<FTPVirtualHostManager> vhost_manager = nullptr,
                  std::shared_ptr<SessionTracker> session_tracker = nullptr);
    ~FTPConnection();

    void start();
    void stop();
    bool isActive() const;

private:
    void handleClient();
    void sendResponse(const std::string& response);
    std::string readLine();
    
    // FTP Command Handlers
    void handleUSER(const std::string& username);
    void handlePASS(const std::string& password);
    void handleQUIT();
    void handlePWD();
    void handleCWD(const std::string& path);
    void handleLIST(const std::string& path);
    void handlePASV();
    void handlePORT(const std::string& address_port);
    void handleEPRT(const std::string& endpoint);
    void handleMODE(const std::string& mode);
    void handleTYPE(const std::string& type);
    void handleSIZE(const std::string& filename);
    void handleRETR(const std::string& filename);
    void handleSTOR(const std::string& filename);
    void handleDELE(const std::string& filename);
    void handleMKD(const std::string& dirname);
    void handleRMD(const std::string& dirname);
    void handleAUTH(const std::string& method);
    void handlePBSZ(const std::string& size);
    void handlePROT(const std::string& level);
    void handleREST(const std::string& position);
    void handleAPPE(const std::string& filename);
    void handleRNFR(const std::string& filename);
    void handleRNTO(const std::string& filename);
    void handleHOST(const std::string& hostname);
    
    // Data Connection Management
    int createPassiveDataSocket();
    int acceptDataConnection();
    int connectActiveDataSocket();
    void closeDataSocket();
    std::string formatPassiveResponse(int port);
    
    // Path and Permission Utilities
    std::string resolvePath(const std::string& path);
    bool validatePath(const std::string& path);
    bool hasPermission(const std::string& operation, const std::string& path);
    bool isPathWithinHome(const std::string& path);

    // SSL/TLS Support
    bool initializeSSL();
    bool upgradeToSSL();
    void* getSSL() const { return ssl_; }
    
    // Security
    void applyChroot();
    bool compressionEnabled() const;
    bool transferModeZ() const { return transfer_mode_ == "Z"; }

    int socket_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<FTPServerConfig> config_;
    std::shared_ptr<FTPUserManager> user_manager_;
    std::shared_ptr<SSLContext> ssl_context_;
    std::shared_ptr<FileCache> file_cache_;
    std::shared_ptr<PAMAuth> pam_auth_;
    
    std::atomic<bool> active_;
    std::thread client_thread_;
    
    bool authenticated_;
    std::string username_;
    std::shared_ptr<FTPUser> current_user_;
    std::string current_directory_;
    
    // SSL/TLS state
    bool ssl_enabled_;
    bool ssl_active_;
    void* ssl_;  // SSL* pointer (void* to avoid OpenSSL dependency in header)
    void* data_ssl_;  // SSL* for data connection
    
    // Data connection state
    int passive_listen_socket_;
    int data_socket_;
    std::mutex data_socket_mutex_;
    std::string transfer_type_;  // "A" for ASCII, "I" for binary
    std::string transfer_mode_;  // "S" stream, "Z" compressed (MODE Z)
    std::string protection_level_;  // "C" for clear, "P" for private (encrypted)
    
    // Active mode state
    std::string active_mode_ip_;
    int active_mode_port_;
    bool active_mode_enabled_;
    
    // Transfer resume state
    std::streampos resume_position_;
    std::string rename_from_path_;
    
    // Virtual hosting (v0.3.0)
    std::shared_ptr<FTPVirtualHostManager> virtual_host_manager_;
    std::shared_ptr<FTPVirtualHost> current_virtual_host_;
    std::shared_ptr<SessionTracker> session_tracker_;
    bool session_registered_ = false;
};

} // namespace simple_sftpd
