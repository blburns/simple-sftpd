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
#include <mutex>

namespace simple_sftpd {

class FTPUser;
class Logger;

class FTPUserManager {
public:
    explicit FTPUserManager(std::shared_ptr<Logger> logger, const std::string& user_file = "");
    ~FTPUserManager() = default;

    bool addUser(std::shared_ptr<FTPUser> user);
    bool removeUser(const std::string& username);
    std::shared_ptr<FTPUser> getUser(const std::string& username);
    
    bool authenticateUser(const std::string& username, const std::string& password);
    std::vector<std::string> listUsers() const;

    // Groups (v0.3.0)
    std::vector<std::string> getUsersInGroup(const std::string& group) const;

    // User persistence
    bool loadUsers(const std::string& filename = "");
    bool saveUsers(const std::string& filename = "") const;
    void setUserFile(const std::string& filename);
    std::string getUserFile() const { return user_file_; }

private:
    std::shared_ptr<Logger> logger_;
    mutable std::mutex users_mutex_;
    std::map<std::string, std::shared_ptr<FTPUser>> users_;
    std::string user_file_;
};

} // namespace simple_sftpd
