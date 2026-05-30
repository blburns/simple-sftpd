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
#include <vector>

namespace simple_sftpd {

class FTPVirtualHost;
class Logger;

class FTPVirtualHostManager {
public:
    explicit FTPVirtualHostManager(std::shared_ptr<Logger> logger,
                                   const std::string& virtual_hosts_file = "");
    ~FTPVirtualHostManager() = default;

    bool addVirtualHost(std::shared_ptr<FTPVirtualHost> host);
    bool removeVirtualHost(const std::string& hostname);
    std::shared_ptr<FTPVirtualHost> getVirtualHost(const std::string& hostname);
    std::vector<std::shared_ptr<FTPVirtualHost>> getAllVirtualHosts() const;

    std::vector<std::string> listVirtualHosts() const;

    bool loadVirtualHosts(const std::string& filename = "");
    bool saveVirtualHosts(const std::string& filename = "") const;
    void setVirtualHostsFile(const std::string& filename);
    std::string getVirtualHostsFile() const { return virtual_hosts_file_; }

private:
    bool persistIfConfigured() const;

    std::shared_ptr<Logger> logger_;
    mutable std::mutex hosts_mutex_;
    std::map<std::string, std::shared_ptr<FTPVirtualHost>> virtual_hosts_;
    std::string virtual_hosts_file_;
};

} // namespace simple_sftpd
