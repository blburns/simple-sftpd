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

namespace simple_sftpd {

class Logger;

/**
 * @brief PAM Authentication
 * 
 * Provides Pluggable Authentication Modules integration
 */
class PAMAuth {
public:
    PAMAuth(std::shared_ptr<Logger> logger);
    ~PAMAuth();

    /**
     * @brief Authenticate user with PAM
     * @param username Username
     * @param password Password
     * @return true if authenticated, false otherwise
     */
    bool authenticate(const std::string& username, const std::string& password);

    /**
     * @brief Check if PAM is available
     * @return true if PAM is available
     */
    bool isAvailable() const { return pam_available_; }

private:
    std::shared_ptr<Logger> logger_;
    bool pam_available_;
};

} // namespace simple_sftpd

