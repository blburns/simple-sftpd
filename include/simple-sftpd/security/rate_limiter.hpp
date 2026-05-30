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
#include <map>
#include <chrono>
#include <mutex>

namespace simple_sftpd {

class Logger;

class FTPRateLimiter {
public:
    explicit FTPRateLimiter(std::shared_ptr<Logger> logger);
    ~FTPRateLimiter() = default;

    bool isAllowed(const std::string& client_ip);
    void recordRequest(const std::string& client_ip);
    void setRateLimit(int max_requests_per_minute);
    void setConnectionLimit(int max_connections_per_ip);

private:
    struct RateLimitEntry {
        int request_count = 0;
        std::chrono::steady_clock::time_point window_start;
    };

    std::shared_ptr<Logger> logger_;
    mutable std::mutex rate_limit_mutex_;
    std::map<std::string, RateLimitEntry> rate_limits_;
    
    int max_requests_per_minute_ = 60;
    int max_connections_per_ip_ = 10;
};

} // namespace simple_sftpd
