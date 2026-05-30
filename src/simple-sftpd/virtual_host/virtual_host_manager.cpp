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

#include "simple-sftpd/virtual_host/virtual_host_manager.hpp"
#include "simple-sftpd/virtual_host/virtual_host.hpp"
#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <string>

#if defined(ENABLE_JSON) || defined(SIMPLE_SFTPD_JSON_ENABLED)
#include <json/json.h>
#define HAS_JSON_SUPPORT 1
#else
#define HAS_JSON_SUPPORT 0
#endif

namespace simple_sftpd {

FTPVirtualHostManager::FTPVirtualHostManager(std::shared_ptr<Logger> logger,
                                               const std::string& virtual_hosts_file)
    : logger_(logger), virtual_hosts_file_(virtual_hosts_file) {
    if (!virtual_hosts_file_.empty()) {
        loadVirtualHosts();
    }
}

void FTPVirtualHostManager::setVirtualHostsFile(const std::string& filename) {
    virtual_hosts_file_ = filename;
}

bool FTPVirtualHostManager::persistIfConfigured() const {
    if (!virtual_hosts_file_.empty()) {
        return saveVirtualHosts();
    }
    return true;
}

bool FTPVirtualHostManager::addVirtualHost(std::shared_ptr<FTPVirtualHost> host) {
    if (!host || host->getHostname().empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(hosts_mutex_);
        if (virtual_hosts_.count(host->getHostname()) > 0) {
            logger_->warn("Virtual host already exists: " + host->getHostname());
            return false;
        }
        virtual_hosts_[host->getHostname()] = host;
    }
    logger_->info("Added virtual host: " + host->getHostname());
    if (!persistIfConfigured()) {
        removeVirtualHost(host->getHostname());
        return false;
    }
    return true;
}

bool FTPVirtualHostManager::removeVirtualHost(const std::string& hostname) {
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(hosts_mutex_);
        auto it = virtual_hosts_.find(hostname);
        if (it != virtual_hosts_.end()) {
            virtual_hosts_.erase(it);
            removed = true;
        }
    }
    if (removed) {
        logger_->info("Removed virtual host: " + hostname);
        if (!persistIfConfigured()) {
            logger_->error("Removed virtual host in memory but failed to save file");
            return false;
        }
        return true;
    }
    return false;
}

std::shared_ptr<FTPVirtualHost> FTPVirtualHostManager::getVirtualHost(const std::string& hostname) {
    std::lock_guard<std::mutex> lock(hosts_mutex_);
    auto it = virtual_hosts_.find(hostname);
    if (it != virtual_hosts_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<FTPVirtualHost>> FTPVirtualHostManager::getAllVirtualHosts() const {
    std::lock_guard<std::mutex> lock(hosts_mutex_);
    std::vector<std::shared_ptr<FTPVirtualHost>> hosts;
    hosts.reserve(virtual_hosts_.size());
    for (const auto& pair : virtual_hosts_) {
        hosts.push_back(pair.second);
    }
    return hosts;
}

std::vector<std::string> FTPVirtualHostManager::listVirtualHosts() const {
    std::lock_guard<std::mutex> lock(hosts_mutex_);
    std::vector<std::string> hostnames;
    for (const auto& pair : virtual_hosts_) {
        hostnames.push_back(pair.first);
    }
    return hostnames;
}

bool FTPVirtualHostManager::loadVirtualHosts(const std::string& filename) {
    std::string file_to_load = filename.empty() ? virtual_hosts_file_ : filename;
    if (file_to_load.empty()) {
        logger_->warn("No virtual hosts file specified for loading");
        return false;
    }

#if HAS_JSON_SUPPORT
    std::ifstream file(file_to_load);
    if (!file.is_open()) {
        logger_->info("Virtual hosts file does not exist yet: " + file_to_load);
        return true;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(file, root)) {
        logger_->error("Failed to parse virtual hosts file: " + reader.getFormattedErrorMessages());
        file.close();
        return false;
    }
    file.close();

    std::map<std::string, std::shared_ptr<FTPVirtualHost>> loaded;

    if (root.isMember("virtual_hosts") && root["virtual_hosts"].isArray()) {
        for (const auto& vh_json : root["virtual_hosts"]) {
            if (!vh_json.isMember("hostname") || !vh_json.isMember("root_directory")) {
                continue;
            }
            std::string hostname = vh_json["hostname"].asString();
            std::string root_dir = vh_json["root_directory"].asString();
            auto host = std::make_shared<FTPVirtualHost>(hostname, root_dir);

            if (vh_json.isMember("enabled")) {
                host->setEnabled(vh_json["enabled"].asBool());
            }
            if (vh_json.isMember("ssl_cert_file")) {
                host->setSslCertFile(vh_json["ssl_cert_file"].asString());
            }
            if (vh_json.isMember("ssl_key_file")) {
                host->setSslKeyFile(vh_json["ssl_key_file"].asString());
            }
            if (vh_json.isMember("ssl_ca_file")) {
                host->setSslCaFile(vh_json["ssl_ca_file"].asString());
            }
            if (vh_json.isMember("max_sessions")) {
                host->setMaxSessions(vh_json["max_sessions"].asInt());
            }
            if (vh_json.isMember("storage_quota_bytes")) {
                host->setStorageQuotaBytes(vh_json["storage_quota_bytes"].asUInt64());
            }
            if (vh_json.isMember("bandwidth_quota_bytes")) {
                host->setBandwidthQuotaBytes(vh_json["bandwidth_quota_bytes"].asUInt64());
            }
            if (vh_json.isMember("user_file")) {
                std::string user_file = vh_json["user_file"].asString();
                if (!user_file.empty()) {
                    host->setUserManager(std::make_shared<FTPUserManager>(logger_, user_file));
                }
            }
            if (vh_json.isMember("custom_errors") && vh_json["custom_errors"].isObject()) {
                for (const auto& key : vh_json["custom_errors"].getMemberNames()) {
                    host->setCustomError(key, vh_json["custom_errors"][key].asString());
                }
            }

            loaded[hostname] = host;
        }
    }

    const size_t count = loaded.size();
    {
        std::lock_guard<std::mutex> lock(hosts_mutex_);
        virtual_hosts_ = std::move(loaded);
    }
    logger_->info("Loaded " + std::to_string(count) + " virtual hosts from " + file_to_load);
    return true;
#else
    logger_->warn("JSON support not enabled. Cannot load virtual hosts from file.");
    (void)file_to_load;
    return false;
#endif
}

bool FTPVirtualHostManager::saveVirtualHosts(const std::string& filename) const {
    std::string file_to_save = filename.empty() ? virtual_hosts_file_ : filename;
    if (file_to_save.empty()) {
        logger_->warn("No virtual hosts file specified for saving");
        return false;
    }

#if HAS_JSON_SUPPORT
    std::filesystem::path file_path(file_to_save);
    if (file_path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(file_path.parent_path(), ec);
        if (ec) {
            logger_->error("Failed to create directory for virtual hosts file: " + ec.message());
            return false;
        }
    }

    Json::Value root;
    Json::Value hosts_array(Json::arrayValue);

    {
        std::lock_guard<std::mutex> lock(hosts_mutex_);
        for (const auto& pair : virtual_hosts_) {
            const auto& host = pair.second;
            Json::Value vh_json;
            vh_json["hostname"] = host->getHostname();
            vh_json["root_directory"] = host->getRootDirectory();
            vh_json["enabled"] = host->isEnabled();
            if (!host->getSslCertFile().empty()) {
                vh_json["ssl_cert_file"] = host->getSslCertFile();
            }
            if (!host->getSslKeyFile().empty()) {
                vh_json["ssl_key_file"] = host->getSslKeyFile();
            }
            if (!host->getSslCaFile().empty()) {
                vh_json["ssl_ca_file"] = host->getSslCaFile();
            }
            if (host->getMaxSessions() > 0) {
                vh_json["max_sessions"] = host->getMaxSessions();
            }
            if (host->getStorageQuotaBytes() > 0) {
                vh_json["storage_quota_bytes"] = static_cast<Json::UInt64>(host->getStorageQuotaBytes());
            }
            if (host->getBandwidthQuotaBytes() > 0) {
                vh_json["bandwidth_quota_bytes"] = static_cast<Json::UInt64>(host->getBandwidthQuotaBytes());
            }
            if (host->getUserManager() && !host->getUserManager()->getUserFile().empty()) {
                vh_json["user_file"] = host->getUserManager()->getUserFile();
            }
            hosts_array.append(vh_json);
        }
    }

    root["virtual_hosts"] = hosts_array;
    root["version"] = "1.0";
    root["updated"] = static_cast<Json::Int64>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    std::ofstream file(file_to_save);
    if (!file.is_open()) {
        logger_->error("Failed to open virtual hosts file for writing: " + file_to_save);
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);
    file.close();

    logger_->info("Saved " + std::to_string(hosts_array.size()) + " virtual hosts to " + file_to_save);
    return true;
#else
    logger_->warn("JSON support not enabled. Cannot save virtual hosts to file.");
    (void)file_to_save;
    return false;
#endif
}

} // namespace simple_sftpd
