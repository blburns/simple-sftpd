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

#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/user/user.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>

#if defined(ENABLE_JSON) || defined(SIMPLE_SFTPD_JSON_ENABLED)
#include <json/json.h>
#define HAS_JSON_SUPPORT 1
#else
#define HAS_JSON_SUPPORT 0
#endif

namespace simple_sftpd {

FTPUserManager::FTPUserManager(std::shared_ptr<Logger> logger, const std::string& user_file)
    : logger_(logger), user_file_(user_file) {
    // Auto-load users if file is specified
    if (!user_file_.empty()) {
        loadUsers();
    }
}

bool FTPUserManager::addUser(std::shared_ptr<FTPUser> user) {
    if (!user) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(users_mutex_);
        users_[user->getUsername()] = user;
    }
    logger_->info("Added user: " + user->getUsername());
    
    // Auto-save if user file is configured
    if (!user_file_.empty()) {
        saveUsers();
    }
    return true;
}

bool FTPUserManager::removeUser(const std::string& username) {
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(users_mutex_);
        auto it = users_.find(username);
        if (it != users_.end()) {
            users_.erase(it);
            removed = true;
        }
    }
    
    if (removed) {
        logger_->info("Removed user: " + username);
        // Auto-save if user file is configured
        if (!user_file_.empty()) {
            saveUsers();
        }
        return true;
    }
    return false;
}

std::shared_ptr<FTPUser> FTPUserManager::getUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(username);
    if (it != users_.end()) {
        return it->second;
    }
    return nullptr;
}

bool FTPUserManager::authenticateUser(const std::string& username, const std::string& password) {
    auto user = getUser(username);
    if (!user) {
        return false;
    }
    return user->authenticate(password);
}

std::vector<std::string> FTPUserManager::listUsers() const {
    std::lock_guard<std::mutex> lock(users_mutex_);
    std::vector<std::string> usernames;
    for (const auto& pair : users_) {
        usernames.push_back(pair.first);
    }
    return usernames;
}

std::vector<std::string> FTPUserManager::getUsersInGroup(const std::string& group) const {
    std::lock_guard<std::mutex> lock(users_mutex_);
    std::vector<std::string> result;
    for (const auto& pair : users_) {
        if (pair.second && pair.second->hasGroup(group)) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void FTPUserManager::setUserFile(const std::string& filename) {
    user_file_ = filename;
}

bool FTPUserManager::loadUsers(const std::string& filename) {
    std::string file_to_load = filename.empty() ? user_file_ : filename;
    if (file_to_load.empty()) {
        logger_->warn("No user file specified for loading");
        return false;
    }

#if HAS_JSON_SUPPORT
    std::ifstream file(file_to_load);
    if (!file.is_open()) {
        // File doesn't exist yet, that's okay - we'll create it on first save
        logger_->info("User file does not exist yet: " + file_to_load);
        return true;
    }

    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(file, root)) {
        logger_->error("Failed to parse user file: " + reader.getFormattedErrorMessages());
        file.close();
        return false;
    }
    file.close();

    std::lock_guard<std::mutex> lock(users_mutex_);
    users_.clear();

    if (root.isMember("users") && root["users"].isArray()) {
        const Json::Value& users_array = root["users"];
        for (const auto& user_json : users_array) {
            if (user_json.isMember("username") && user_json.isMember("password") &&
                user_json.isMember("home_directory")) {
                std::string username = user_json["username"].asString();
                std::string password = user_json["password"].asString();
                std::string home_dir = user_json["home_directory"].asString();

                auto user = std::make_shared<FTPUser>(username, password, home_dir);
                if (user_json.isMember("groups") && user_json["groups"].isArray()) {
                    std::vector<std::string> groups;
                    for (const auto& g : user_json["groups"]) {
                        groups.push_back(g.asString());
                    }
                    user->setGroups(groups);
                }
                if (user_json.isMember("is_guest")) {
                    user->setGuest(user_json["is_guest"].asBool());
                }
                if (user_json.isMember("expires_at")) {
                    user->setExpiresAt(user_json["expires_at"].asInt64());
                }
                if (user_json.isMember("storage_quota_bytes")) {
                    user->setStorageQuotaBytes(static_cast<uint64_t>(user_json["storage_quota_bytes"].asUInt64()));
                }
                users_[username] = user;
            }
        }
        logger_->info("Loaded " + std::to_string(users_.size()) + " users from " + file_to_load);
        return true;
    } else {
        logger_->warn("User file does not contain valid users array");
        return false;
    }
#else
    logger_->warn("JSON support not enabled. Cannot load users from file.");
    return false;
#endif
}

bool FTPUserManager::saveUsers(const std::string& filename) const {
    std::string file_to_save = filename.empty() ? user_file_ : filename;
    if (file_to_save.empty()) {
        logger_->warn("No user file specified for saving");
        return false;
    }

#if HAS_JSON_SUPPORT
    // Create directory if it doesn't exist
    std::filesystem::path file_path(file_to_save);
    if (file_path.has_parent_path()) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    Json::Value root;
    Json::Value users_array(Json::arrayValue);

    {
        std::lock_guard<std::mutex> lock(users_mutex_);
        for (const auto& pair : users_) {
            const auto& user = pair.second;
            Json::Value user_json;
            user_json["username"] = user->getUsername();
            user_json["password"] = user->getPassword(); // Note: In production, this should be hashed
            user_json["home_directory"] = user->getHomeDirectory();
            Json::Value groups_arr(Json::arrayValue);
            for (const auto& g : user->getGroups()) {
                groups_arr.append(g);
            }
            user_json["groups"] = groups_arr;
            user_json["is_guest"] = user->isGuest();
            user_json["expires_at"] = static_cast<Json::Int64>(user->getExpiresAt());
            user_json["storage_quota_bytes"] = static_cast<Json::UInt64>(user->getStorageQuotaBytes());
            users_array.append(user_json);
        }
    }

    root["users"] = users_array;
    root["version"] = "1.0";
    root["updated"] = static_cast<Json::Int64>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    std::ofstream file(file_to_save);
    if (!file.is_open()) {
        logger_->error("Failed to open user file for writing: " + file_to_save);
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);
    file.close();

    logger_->info("Saved " + std::to_string(users_array.size()) + " users to " + file_to_save);
    return true;
#else
    logger_->warn("JSON support not enabled. Cannot save users to file.");
    return false;
#endif
}

} // namespace simple_sftpd
