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
#include "simple-sftpd/security/password.hpp"
#include <chrono>
#include <algorithm>
#include <filesystem>

namespace simple_sftpd {

namespace {

std::filesystem::path normalize(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec || normalized.empty()) {
        normalized = path.lexically_normal();
    }
    return normalized;
}

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

FTPUser::FTPUser(const std::string& username, const std::string& password, const std::string& home_dir)
    : username_(username), home_directory_(home_dir) {
    setPassword(password);
}

void FTPUser::setPassword(const std::string& password) {
    if (password::isHashed(password)) {
        password_ = password;
        return;
    }

    std::string hashed = password::hash(password);
    if (hashed.empty()) {
        // The RNG failed. Keep the value rather than silently dropping the
        // credential; hasLegacyPassword() will report it as un-migrated.
        password_ = password;
        return;
    }
    password_ = std::move(hashed);
}

bool FTPUser::hasLegacyPassword() const {
    return !password::isHashed(password_);
}

bool FTPUser::authenticate(const std::string& password) const {
    if (isExpired()) {
        return false;
    }
    return password::verify(password, password_);
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

void FTPUser::addPermission(const std::string& permission) {
    if (std::find(permissions_.begin(), permissions_.end(), permission) == permissions_.end()) {
        permissions_.push_back(permission);
    }
}

bool FTPUser::hasPermission(const std::string& operation, const std::string& path) const {
    // An empty permission list means the account is unrestricted.
    if (permissions_.empty()) {
        return true;
    }

    for (const auto& entry : permissions_) {
        const auto separator = entry.find(':');
        const std::string allowed_operation =
            separator == std::string::npos ? entry : entry.substr(0, separator);

        if (allowed_operation != operation && allowed_operation != "all") {
            continue;
        }

        if (separator == std::string::npos) {
            return true; // Unscoped: applies everywhere in the account.
        }

        const std::string scope = entry.substr(separator + 1);
        if (scope.empty() || scope == "/") {
            return true;
        }
        if (path.empty()) {
            continue; // Caller gave no path, so a scoped grant cannot apply.
        }

        // Scopes are virtual paths rooted at the user's home directory.
        const std::filesystem::path scope_root =
            normalize(std::filesystem::path(home_directory_) /
                      std::filesystem::path(scope).relative_path());
        if (isWithin(normalize(path), scope_root)) {
            return true;
        }
    }

    return false;
}

} // namespace simple_sftpd
