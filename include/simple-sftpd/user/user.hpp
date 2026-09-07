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
#include <chrono>
#include <cstdint>

namespace simple_sftpd {

class FTPUser {
public:
    FTPUser() = default;
    FTPUser(const std::string& username, const std::string& password, const std::string& home_dir);
    ~FTPUser() = default;

    const std::string& getUsername() const { return username_; }
    const std::string& getHomeDirectory() const { return home_directory_; }

    /**
     * The stored credential: normally a salted PBKDF2 hash, or a legacy
     * plaintext value read from an un-migrated user file. Intended for
     * serialization only -- use authenticate() to check a password.
     */
    const std::string& getPasswordHash() const { return password_; }

    /// True when the stored credential is still legacy plaintext.
    bool hasLegacyPassword() const;

    void setUsername(const std::string& username) { username_ = username; }
    void setHomeDirectory(const std::string& home_dir) { home_directory_ = home_dir; }

    /**
     * Accepts either a plaintext password or an already-hashed credential.
     * Plaintext is hashed before storage, so loading an existing user file
     * migrates it in place.
     */
    void setPassword(const std::string& password);

    bool authenticate(const std::string& password) const;

    /**
     * Permission entries are either a bare operation ("read", "write", "list",
     * "all") or an operation scoped to a subtree ("write:/uploads"). Scopes are
     * virtual paths rooted at the user's home directory. An empty permission
     * list means unrestricted.
     */
    bool hasPermission(const std::string& operation, const std::string& path) const;
    const std::vector<std::string>& getPermissions() const { return permissions_; }
    void setPermissions(const std::vector<std::string>& permissions) { permissions_ = permissions; }
    void addPermission(const std::string& permission);

    // Groups (v0.3.0)
    const std::vector<std::string>& getGroups() const { return groups_; }
    void setGroups(const std::vector<std::string>& g) { groups_ = g; }
    void addGroup(const std::string& g);
    bool hasGroup(const std::string& group) const;

    // Guest / expiry (v0.3.0)
    bool isGuest() const { return is_guest_; }
    void setGuest(bool g) { is_guest_ = g; }
    int64_t getExpiresAt() const { return expires_at_; }
    void setExpiresAt(int64_t t) { expires_at_ = t; }
    bool isExpired() const;

    // Per-user storage quota (v0.3.0); 0 = unlimited
    uint64_t getStorageQuotaBytes() const { return storage_quota_bytes_; }
    void setStorageQuotaBytes(uint64_t n) { storage_quota_bytes_ = n; }

private:
    std::string username_;
    std::string password_;
    std::string home_directory_;
    std::vector<std::string> permissions_;
    std::vector<std::string> groups_;
    bool is_guest_ = false;
    int64_t expires_at_ = 0;  // seconds since epoch; 0 = no expiry
    uint64_t storage_quota_bytes_ = 0;
};

} // namespace simple_sftpd
