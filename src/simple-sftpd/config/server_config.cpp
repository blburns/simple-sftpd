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

#include "simple-sftpd/config/server_config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#if defined(ENABLE_JSON) || defined(SIMPLE_SFTPD_JSON_ENABLED)
#include <json/json.h>
#define HAS_JSON_SUPPORT 1
#else
#define HAS_JSON_SUPPORT 0
#endif

namespace simple_sftpd {

bool FTPServerConfig::loadFromFile(const std::string& filename) {
    clearErrors();
    
    // Set defaults first
    connection.bind_address = "0.0.0.0";
    connection.bind_port = 21;
    connection.max_connections = 100;
    connection.timeout_seconds = 300;
    
    logging.log_file = "/var/log/simple-sftpd/simple-sftpd.log";
    logging.log_level = "INFO";
    logging.log_to_console = true;
    logging.log_to_file = true;
    
    security.require_ssl = false;
    security.allow_anonymous = false;
    
    rate_limit.enabled = false;
    rate_limit.max_requests_per_minute = 60;
    rate_limit.max_connections_per_ip = 10;
    
    // Check if file exists
    if (!std::filesystem::exists(filename)) {
        addWarning("Configuration file not found: " + filename + ", using defaults");
        return true; // Not an error, just use defaults
    }
    
    // Detect file format from extension
    std::filesystem::path file_path(filename);
    std::string extension = file_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Try to load based on format
    if (extension == ".json") {
        return loadFromJSON(filename);
    } else if (extension == ".yml" || extension == ".yaml") {
        return loadFromYAML(filename);
    } else {
        // Default to INI format (.conf or no extension)
        return loadFromINI(filename);
    }
}

bool FTPServerConfig::loadFromINI(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        addError("Failed to open configuration file: " + filename);
        return false;
    }
    
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) {
            continue;
        }
        
        // Check for section header [section]
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key = value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }
        
        // Parse based on section and key
        if (current_section == "connection" || current_section.empty()) {
            if (key == "bind_address") {
                connection.bind_address = value;
            } else if (key == "bind_port") {
                connection.bind_port = std::stoi(value);
            } else if (key == "max_connections") {
                connection.max_connections = std::stoi(value);
            } else if (key == "timeout_seconds" || key == "connection_timeout") {
                connection.timeout_seconds = std::stoi(value);
            }
        } else if (current_section == "logging") {
            if (key == "log_file") {
                logging.log_file = value;
            } else if (key == "log_level") {
                logging.log_level = value;
            } else if (key == "log_format") {
                logging.log_format = value;
            } else if (key == "log_to_console") {
                logging.log_to_console = (value == "true" || value == "1");
            } else if (key == "log_to_file") {
                logging.log_to_file = (value == "true" || value == "1");
            }
        } else if (current_section == "security") {
            if (key == "max_sessions_per_user") {
                security.max_sessions_per_user = std::stoi(value);
            } else if (key == "require_ssl") {
                security.require_ssl = (value == "true" || value == "1");
            } else if (key == "allow_anonymous") {
                security.allow_anonymous = (value == "true" || value == "1");
            } else if (key == "ssl_cert_file" || key == "certificate_file") {
                security.ssl_cert_file = value;
            } else if (key == "ssl_key_file" || key == "private_key_file") {
                security.ssl_key_file = value;
            } else if (key == "chroot_enabled") {
                security.chroot_enabled = (value == "true" || value == "1");
            } else if (key == "chroot_directory") {
                security.chroot_directory = value;
            } else if (key == "drop_privileges") {
                security.drop_privileges = (value == "true" || value == "1");
            } else if (key == "run_as_user") {
                security.run_as_user = value;
            } else if (key == "run_as_group") {
                security.run_as_group = value;
            } else if (key == "ssl_ca_file" || key == "ca_certificate_file") {
                security.ssl_ca_file = value;
            } else if (key == "require_client_cert") {
                security.require_client_cert = (value == "true" || value == "1");
            } else if (key == "ssl_client_ca_file") {
                security.ssl_client_ca_file = value;
            } else if (key == "enable_pam") {
                security.enable_pam = (value == "true" || value == "1");
            } else if (key == "user_file") {
                security.user_file = value;
            }
        } else if (current_section == "rate_limit") {
            if (key == "enabled") {
                rate_limit.enabled = (value == "true" || value == "1");
            } else if (key == "max_requests_per_minute") {
                rate_limit.max_requests_per_minute = std::stoi(value);
            } else if (key == "max_connections_per_ip") {
                rate_limit.max_connections_per_ip = std::stoi(value);
            } else if (key == "max_transfer_rate") {
                rate_limit.max_transfer_rate = std::stoi(value);
            } else if (key == "max_transfer_rate_per_user") {
                rate_limit.max_transfer_rate_per_user = std::stoi(value);
            }
        } else if (current_section == "transfer") {
            if (key == "use_sendfile") {
                transfer.use_sendfile = (value == "true" || value == "1");
            } else if (key == "use_mmap") {
                transfer.use_mmap = (value == "true" || value == "1");
            } else if (key == "buffer_size") {
                transfer.buffer_size = static_cast<size_t>(std::stoul(value));
            }
        }
    }
    
    file.close();
    return true;
}

bool FTPServerConfig::loadFromJSON(const std::string& filename) {
#if HAS_JSON_SUPPORT
    std::ifstream file(filename);
    if (!file.is_open()) {
        addError("Failed to open JSON configuration file: " + filename);
        return false;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(file, root)) {
        addError("Failed to parse JSON configuration: " + reader.getFormattedErrorMessages());
        file.close();
        return false;
    }
    file.close();
    
    // Parse connection section
    if (root.isMember("connection")) {
        const Json::Value& conn = root["connection"];
        if (conn.isMember("bind_address")) connection.bind_address = conn["bind_address"].asString();
        if (conn.isMember("bind_port")) connection.bind_port = conn["bind_port"].asInt();
        if (conn.isMember("max_connections")) connection.max_connections = conn["max_connections"].asInt();
        if (conn.isMember("timeout_seconds")) connection.timeout_seconds = conn["timeout_seconds"].asInt();
        if (conn.isMember("passive_mode")) connection.passive_mode = conn["passive_mode"].asBool();
        if (conn.isMember("passive_port_range_start")) connection.passive_port_range_start = conn["passive_port_range_start"].asInt();
        if (conn.isMember("passive_port_range_end")) connection.passive_port_range_end = conn["passive_port_range_end"].asInt();
    }
    
    // Parse logging section
    if (root.isMember("logging")) {
        const Json::Value& log = root["logging"];
        if (log.isMember("log_file")) logging.log_file = log["log_file"].asString();
        if (log.isMember("log_level")) logging.log_level = log["log_level"].asString();
        if (log.isMember("log_format")) logging.log_format = log["log_format"].asString();
        if (log.isMember("log_to_console")) logging.log_to_console = log["log_to_console"].asBool();
        if (log.isMember("log_to_file")) logging.log_to_file = log["log_to_file"].asBool();
    }
    
    // Parse security section
    if (root.isMember("security")) {
        const Json::Value& sec = root["security"];
        if (sec.isMember("max_sessions_per_user")) {
            security.max_sessions_per_user = sec["max_sessions_per_user"].asInt();
        }
        if (sec.isMember("require_ssl")) security.require_ssl = sec["require_ssl"].asBool();
        if (sec.isMember("allow_anonymous")) security.allow_anonymous = sec["allow_anonymous"].asBool();
        if (sec.isMember("ssl_cert_file")) security.ssl_cert_file = sec["ssl_cert_file"].asString();
        if (sec.isMember("ssl_key_file")) security.ssl_key_file = sec["ssl_key_file"].asString();
        if (sec.isMember("ssl_ca_file")) security.ssl_ca_file = sec["ssl_ca_file"].asString();
        if (sec.isMember("require_client_cert")) security.require_client_cert = sec["require_client_cert"].asBool();
        if (sec.isMember("ssl_client_ca_file")) security.ssl_client_ca_file = sec["ssl_client_ca_file"].asString();
        if (sec.isMember("chroot_enabled")) security.chroot_enabled = sec["chroot_enabled"].asBool();
        if (sec.isMember("chroot_directory")) security.chroot_directory = sec["chroot_directory"].asString();
        if (sec.isMember("drop_privileges")) security.drop_privileges = sec["drop_privileges"].asBool();
        if (sec.isMember("run_as_user")) security.run_as_user = sec["run_as_user"].asString();
        if (sec.isMember("run_as_group")) security.run_as_group = sec["run_as_group"].asString();
        if (sec.isMember("enable_pam")) security.enable_pam = sec["enable_pam"].asBool();
        if (sec.isMember("user_file")) security.user_file = sec["user_file"].asString();
    }
    
    // Parse rate_limit section
    if (root.isMember("rate_limit")) {
        const Json::Value& rate = root["rate_limit"];
        if (rate.isMember("enabled")) rate_limit.enabled = rate["enabled"].asBool();
        if (rate.isMember("max_requests_per_minute")) rate_limit.max_requests_per_minute = rate["max_requests_per_minute"].asInt();
        if (rate.isMember("max_connections_per_ip")) rate_limit.max_connections_per_ip = rate["max_connections_per_ip"].asInt();
        if (rate.isMember("max_transfer_rate")) rate_limit.max_transfer_rate = rate["max_transfer_rate"].asInt();
        if (rate.isMember("max_transfer_rate_per_user")) rate_limit.max_transfer_rate_per_user = rate["max_transfer_rate_per_user"].asInt();
    }
    
    // Parse transfer section
    if (root.isMember("transfer")) {
        const Json::Value& tr = root["transfer"];
        if (tr.isMember("use_sendfile")) transfer.use_sendfile = tr["use_sendfile"].asBool();
        if (tr.isMember("use_mmap")) transfer.use_mmap = tr["use_mmap"].asBool();
        if (tr.isMember("buffer_size")) transfer.buffer_size = static_cast<size_t>(tr["buffer_size"].asUInt());
    }
    
    return true;
#else
    addError("JSON support not enabled. Rebuild with ENABLE_JSON=ON");
    return false;
#endif
}

bool FTPServerConfig::loadFromYAML(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        addError("Failed to open YAML configuration file: " + filename);
        return false;
    }
    
    // Simple YAML parser (basic key-value and nested structure)
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // Calculate indentation
        size_t first_non_space = line.find_first_not_of(" \t");
        if (first_non_space == std::string::npos) {
            continue; // Empty line
        }
        
        line = line.substr(first_non_space);
        
        // Handle section changes (key ending with :)
        if (line.back() == ':') {
            current_section = line.substr(0, line.length() - 1);
            // Trim whitespace from section name
            current_section.erase(0, current_section.find_first_not_of(" \t"));
            current_section.erase(current_section.find_last_not_of(" \t") + 1);
            continue;
        }
        
        // Parse key: value
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (value.length() >= 2 && ((value[0] == '"' && value.back() == '"') || 
                                    (value[0] == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }
        
        // Parse based on section and key (same logic as INI)
        if (current_section == "connection") {
            if (key == "bind_address") {
                connection.bind_address = value;
            } else if (key == "bind_port") {
                connection.bind_port = std::stoi(value);
            } else if (key == "max_connections") {
                connection.max_connections = std::stoi(value);
            } else if (key == "timeout_seconds") {
                connection.timeout_seconds = std::stoi(value);
            } else if (key == "passive_mode") {
                connection.passive_mode = (value == "true" || value == "1");
            } else if (key == "passive_port_range_start") {
                connection.passive_port_range_start = std::stoi(value);
            } else if (key == "passive_port_range_end") {
                connection.passive_port_range_end = std::stoi(value);
            }
        } else if (current_section == "logging") {
            if (key == "log_file") {
                logging.log_file = value;
            } else if (key == "log_level") {
                logging.log_level = value;
            } else if (key == "log_format") {
                logging.log_format = value;
            } else if (key == "log_to_console") {
                logging.log_to_console = (value == "true" || value == "1");
            } else if (key == "log_to_file") {
                logging.log_to_file = (value == "true" || value == "1");
            }
        } else if (current_section == "security") {
            if (key == "max_sessions_per_user") {
                security.max_sessions_per_user = std::stoi(value);
            } else if (key == "require_ssl") {
                security.require_ssl = (value == "true" || value == "1");
            } else if (key == "allow_anonymous") {
                security.allow_anonymous = (value == "true" || value == "1");
            } else if (key == "ssl_cert_file" || key == "certificate_file") {
                security.ssl_cert_file = value;
            } else if (key == "ssl_key_file" || key == "private_key_file") {
                security.ssl_key_file = value;
            } else if (key == "ssl_ca_file" || key == "ca_certificate_file") {
                security.ssl_ca_file = value;
            } else if (key == "require_client_cert") {
                security.require_client_cert = (value == "true" || value == "1");
            } else if (key == "ssl_client_ca_file") {
                security.ssl_client_ca_file = value;
            } else if (key == "chroot_enabled") {
                security.chroot_enabled = (value == "true" || value == "1");
            } else if (key == "chroot_directory") {
                security.chroot_directory = value;
            } else if (key == "drop_privileges") {
                security.drop_privileges = (value == "true" || value == "1");
            } else if (key == "run_as_user") {
                security.run_as_user = value;
            } else if (key == "run_as_group") {
                security.run_as_group = value;
            } else if (key == "enable_pam") {
                security.enable_pam = (value == "true" || value == "1");
            } else if (key == "user_file") {
                security.user_file = value;
            }
        } else if (current_section == "rate_limit") {
            if (key == "enabled") {
                rate_limit.enabled = (value == "true" || value == "1");
            } else if (key == "max_requests_per_minute") {
                rate_limit.max_requests_per_minute = std::stoi(value);
            } else if (key == "max_connections_per_ip") {
                rate_limit.max_connections_per_ip = std::stoi(value);
            } else if (key == "max_transfer_rate") {
                rate_limit.max_transfer_rate = std::stoi(value);
            } else if (key == "max_transfer_rate_per_user") {
                rate_limit.max_transfer_rate_per_user = std::stoi(value);
            }
        } else if (current_section == "transfer") {
            if (key == "use_sendfile") {
                transfer.use_sendfile = (value == "true" || value == "1");
            } else if (key == "use_mmap") {
                transfer.use_mmap = (value == "true" || value == "1");
            } else if (key == "buffer_size") {
                transfer.buffer_size = static_cast<size_t>(std::stoul(value));
            }
        }
    }
    
    file.close();
    return true;
}

bool FTPServerConfig::validate() {
    clearErrors();
    
    // Basic validation
    if (connection.bind_port <= 0 || connection.bind_port > 65535) {
        addError("Invalid bind port: " + std::to_string(connection.bind_port));
    }
    
    if (connection.max_connections <= 0) {
        addError("Invalid max connections: " + std::to_string(connection.max_connections));
    }
    
    if (connection.timeout_seconds <= 0) {
        addError("Invalid timeout: " + std::to_string(connection.timeout_seconds));
    }
    
    return errors_.empty();
}

void FTPServerConfig::clearErrors() {
    errors_.clear();
    warnings_.clear();
}

void FTPServerConfig::addError(const std::string& error) {
    errors_.push_back(error);
}

void FTPServerConfig::addWarning(const std::string& warning) {
    warnings_.push_back(warning);
}

} // namespace simple_sftpd
