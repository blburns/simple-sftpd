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
#include <memory>
#include <map>

namespace simple_sftpd {

class FTPUserManager;

class FTPVirtualHost {
public:
    FTPVirtualHost() = default;
    FTPVirtualHost(const std::string& hostname, const std::string& root_directory);
    ~FTPVirtualHost() = default;

    const std::string& getHostname() const { return hostname_; }
    const std::string& getRootDirectory() const { return root_directory_; }
    bool isEnabled() const { return enabled_; }
    
    void setHostname(const std::string& hostname) { hostname_ = hostname; }
    void setRootDirectory(const std::string& root_dir) { root_directory_ = root_dir; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

    std::shared_ptr<FTPUserManager> getUserManager() const { return user_manager_; }
    void setUserManager(std::shared_ptr<FTPUserManager> user_manager) { user_manager_ = user_manager; }

    // Per-host SSL (v0.3.0)
    const std::string& getSslCertFile() const { return ssl_cert_file_; }
    const std::string& getSslKeyFile() const { return ssl_key_file_; }
    const std::string& getSslCaFile() const { return ssl_ca_file_; }
    void setSslCertFile(const std::string& s) { ssl_cert_file_ = s; }
    void setSslKeyFile(const std::string& s) { ssl_key_file_ = s; }
    void setSslCaFile(const std::string& s) { ssl_ca_file_ = s; }

    // Resource isolation (v0.3.0)
    int getMaxSessions() const { return max_sessions_; }
    uint64_t getStorageQuotaBytes() const { return storage_quota_bytes_; }
    uint64_t getBandwidthQuotaBytes() const { return bandwidth_quota_bytes_; }
    void setMaxSessions(int n) { max_sessions_ = n; }
    void setStorageQuotaBytes(uint64_t n) { storage_quota_bytes_ = n; }
    void setBandwidthQuotaBytes(uint64_t n) { bandwidth_quota_bytes_ = n; }

    // Custom error messages (v0.3.0): code e.g. "530" -> message
    std::string getCustomError(const std::string& code) const;
    void setCustomError(const std::string& code, const std::string& message);

private:
    std::string hostname_;
    std::string root_directory_;
    bool enabled_ = true;
    std::shared_ptr<FTPUserManager> user_manager_;
    std::string ssl_cert_file_;
    std::string ssl_key_file_;
    std::string ssl_ca_file_;
    int max_sessions_ = 0;           // 0 = unlimited
    uint64_t storage_quota_bytes_ = 0;   // 0 = unlimited
    uint64_t bandwidth_quota_bytes_ = 0; // 0 = unlimited
    std::map<std::string, std::string> custom_errors_;
};

} // namespace simple_sftpd
