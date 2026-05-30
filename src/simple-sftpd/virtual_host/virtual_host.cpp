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

#include "simple-sftpd/virtual_host/virtual_host.hpp"

namespace simple_sftpd {

FTPVirtualHost::FTPVirtualHost(const std::string& hostname, const std::string& root_directory)
    : hostname_(hostname), root_directory_(root_directory), enabled_(true) {
}

std::string FTPVirtualHost::getCustomError(const std::string& code) const {
    auto it = custom_errors_.find(code);
    return it != custom_errors_.end() ? it->second : "";
}

void FTPVirtualHost::setCustomError(const std::string& code, const std::string& message) {
    if (message.empty()) {
        custom_errors_.erase(code);
    } else {
        custom_errors_[code] = message;
    }
}

} // namespace simple_sftpd
