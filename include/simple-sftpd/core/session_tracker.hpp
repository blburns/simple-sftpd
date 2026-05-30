/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include <string>
#include <mutex>
#include <map>

namespace simple_sftpd {

/**
 * Tracks active sessions per user and per virtual host for v0.3.0 session limits.
 */
class SessionTracker {
public:
    SessionTracker() = default;

    void registerSession(const std::string& username, const std::string& hostname);
    void unregisterSession(const std::string& username, const std::string& hostname);

    size_t getCountForUser(const std::string& username) const;
    size_t getCountForHost(const std::string& hostname) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, size_t> count_by_user_;
    std::map<std::string, size_t> count_by_host_;
};

} // namespace simple_sftpd
