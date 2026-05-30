/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "simple-sftpd/core/session_tracker.hpp"

namespace simple_sftpd {

void SessionTracker::registerSession(const std::string& username, const std::string& hostname) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!username.empty()) {
        count_by_user_[username]++;
    }
    if (!hostname.empty()) {
        count_by_host_[hostname]++;
    }
}

void SessionTracker::unregisterSession(const std::string& username, const std::string& hostname) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!username.empty()) {
        auto it = count_by_user_.find(username);
        if (it != count_by_user_.end()) {
            if (it->second <= 1) {
                count_by_user_.erase(it);
            } else {
                it->second--;
            }
        }
    }
    if (!hostname.empty()) {
        auto it = count_by_host_.find(hostname);
        if (it != count_by_host_.end()) {
            if (it->second <= 1) {
                count_by_host_.erase(it);
            } else {
                it->second--;
            }
        }
    }
}

size_t SessionTracker::getCountForUser(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = count_by_user_.find(username);
    return it != count_by_user_.end() ? it->second : 0;
}

size_t SessionTracker::getCountForHost(const std::string& hostname) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = count_by_host_.find(hostname);
    return it != count_by_host_.end() ? it->second : 0;
}

} // namespace simple_sftpd
