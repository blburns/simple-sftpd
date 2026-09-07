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

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace simple_sftpd {

struct ConnectionConfig {
    std::string bind_address = "0.0.0.0";
    int bind_port = 21;
    int max_connections = 100;
    int timeout_seconds = 300;
    bool passive_mode = true;
    int passive_port_range_start = 49152;
    int passive_port_range_end = 65535;
};

struct LoggingConfig {
    std::string log_file = "/var/log/simple-sftpd/simple-sftpd.log";
    std::string log_level = "INFO";
    std::string log_format = "STANDARD";  // STANDARD, JSON, or EXTENDED
    bool log_to_console = true;
    bool log_to_file = true;
};

struct SecurityConfig {
    int max_sessions_per_user = 0;  // 0 = unlimited (v0.3.0)
    bool require_ssl = false;
    std::string ssl_cert_file;
    std::string ssl_key_file;
    std::string ssl_ca_file;
    bool require_client_cert = false;
    std::string ssl_client_ca_file;
    bool allow_anonymous = false;
    std::string anonymous_user = "anonymous";
    std::string anonymous_password = "anonymous@";
    bool chroot_enabled = false;
    std::string chroot_directory = "/var/ftp";
    bool drop_privileges = false;
    std::string run_as_user = "ftp";
    std::string run_as_group = "ftp";
    bool enable_pam = false;
    std::string user_file;  // JSON file for persistent user storage; empty = in-memory only
    std::string virtual_hosts_file;  // JSON file for virtual host definitions
};

struct RateLimitConfig {
    bool enabled = false;
    int max_requests_per_minute = 60;
    int max_connections_per_ip = 10;
    int max_transfer_rate = 0;  // bytes per second, 0 = unlimited
    int max_transfer_rate_per_user = 0;  // bytes per second per user
};

struct TransferConfig {
    bool use_sendfile = false;  // use sendfile() for RETR when available
    bool use_mmap = false;      // use mmap for RETR when sendfile not used
    bool enable_compression = false;  // allow MODE Z on the wire
    size_t buffer_size = 32768;
};

class FTPServerConfig {
public:
    FTPServerConfig() = default;
    ~FTPServerConfig() = default;

    bool loadFromFile(const std::string& filename);
    bool validate();
    
    // Format-specific loaders
    bool loadFromINI(const std::string& filename);
    bool loadFromJSON(const std::string& filename);
    bool loadFromYAML(const std::string& filename);
    
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }

    // Configuration sections
    ConnectionConfig connection;
    LoggingConfig logging;
    SecurityConfig security;
    RateLimitConfig rate_limit;
    TransferConfig transfer;

private:
    void clearErrors();
    void addError(const std::string& error);
    void addWarning(const std::string& warning);

    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
};

} // namespace simple_sftpd
