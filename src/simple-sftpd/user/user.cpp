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

#include "simple-sftpd/user/user.hpp"
#include <chrono>
#include <algorithm>

namespace simple_sftpd {

FTPUser::FTPUser(const std::string& username, const std::string& password, const std::string& home_dir)
    : username_(username), password_(password), home_directory_(home_dir) {
}

bool FTPUser::authenticate(const std::string& password) const {
    if (isExpired()) {
        return false;
    }
    return password_ == password;
}

void FTPUser::addGroup(const std::string& g) {
    if (std::find(groups_.begin(), groups_.end(), g) == groups_.end()) {
        groups_.push_back(g);
    }
}

bool FTPUser::hasGroup(const std::string& group) const {
    return std::find(groups_.begin(), groups_.end(), group) != groups_.end();
}

bool FTPUser::isExpired() const {
    if (expires_at_ <= 0) {
        return false;
    }
    int64_t now = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    return now > expires_at_;
}

bool FTPUser::hasPermission(const std::string& operation, const std::string& path) const {
    (void)operation; // Suppress unused parameter warning
    (void)path; // Suppress unused parameter warning
    
    // Basic permission implementation
    // For v0.1.0, we implement simple permission checks
    // More advanced permission system will be in v0.2.0
    
    // If permissions_ vector is empty, allow all (backward compatibility)
    if (permissions_.empty()) {
        return true;
    }
    
    // Check if operation is in permissions list
    for (const auto& perm : permissions_) {
        if (perm == operation || perm == "all") {
            return true;
        }
    }
    
    return false;
}

} // namespace simple_sftpd
