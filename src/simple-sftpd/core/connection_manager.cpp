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

#include "simple-sftpd/core/connection_manager.hpp"
#include "simple-sftpd/core/connection.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include <atomic>
#include <algorithm>

namespace simple_sftpd {

FTPConnectionManager::FTPConnectionManager(std::shared_ptr<FTPServerConfig> config,
                                         std::shared_ptr<Logger> logger)
    : config_(config), logger_(logger), running_(false),
      connection_timeout_(std::chrono::seconds(300)),
      cleanup_interval_(std::chrono::seconds(60)),
      pool_size_(10) {
}

FTPConnectionManager::~FTPConnectionManager() {
    stop();
}

bool FTPConnectionManager::start() {
    if (running_) {
        return true;
    }
    
    running_ = true;
    cleanup_thread_ = std::thread(&FTPConnectionManager::cleanupLoop, this);
    pool_maintenance_thread_ = std::thread(&FTPConnectionManager::poolMaintenanceLoop, this);
    logger_->info("FTP connection manager started");
    return true;
}

void FTPConnectionManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    if (pool_maintenance_thread_.joinable()) {
        pool_maintenance_thread_.join();
    }
    stopAllConnections();
    
    // Clear connection pool
    std::lock_guard<std::mutex> pool_lock(pool_mutex_);
    connection_pool_.clear();
    
    logger_->info("FTP connection manager stopped");
}

void FTPConnectionManager::addConnection(std::shared_ptr<FTPConnection> connection) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.push_back(connection);
}

void FTPConnectionManager::removeConnection(std::shared_ptr<FTPConnection> connection) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(
        std::remove(connections_.begin(), connections_.end(), connection),
        connections_.end()
    );
}

void FTPConnectionManager::stopAllConnections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& connection : connections_) {
        if (connection) {
            connection->stop();
        }
    }
    connections_.clear();
}

size_t FTPConnectionManager::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

std::vector<std::shared_ptr<FTPConnection>> FTPConnectionManager::getConnections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_;
}

void FTPConnectionManager::cleanupLoop() {
    while (running_) {
        for (int i = 0; i < 20 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!running_) {
            break;
        }
        
        // Clean up inactive connections
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_.begin();
        while (it != connections_.end()) {
            if (!(*it) || !(*it)->isActive()) {
                if (*it) {
                    (*it)->stop();
                }
                it = connections_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::shared_ptr<FTPConnection> FTPConnectionManager::acquireConnection() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Try to get a connection from the pool
    if (!connection_pool_.empty()) {
        auto connection = connection_pool_.back();
        connection_pool_.pop_back();
        if (connection && connection->isActive()) {
            return connection;
        }
    }
    
    // Pool is empty or connection is invalid, return nullptr
    // Caller should create a new connection
    return nullptr;
}

void FTPConnectionManager::releaseConnection(std::shared_ptr<FTPConnection> connection) {
    if (!connection || !connection->isActive()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Add to pool if there's room
    if (connection_pool_.size() < pool_size_) {
        connection_pool_.push_back(connection);
    } else {
        // Pool is full, stop the connection
        connection->stop();
    }
}

void FTPConnectionManager::setPoolSize(size_t pool_size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    pool_size_ = pool_size;
    
    // Trim pool if necessary
    while (connection_pool_.size() > pool_size_) {
        auto connection = connection_pool_.back();
        connection_pool_.pop_back();
        if (connection) {
            connection->stop();
        }
    }
}

void FTPConnectionManager::poolMaintenanceLoop() {
    while (running_) {
        for (int i = 0; i < 20 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!running_) {
            break;
        }
        
        // Clean up inactive connections from pool
        std::lock_guard<std::mutex> lock(pool_mutex_);
        auto it = connection_pool_.begin();
        while (it != connection_pool_.end()) {
            if (!(*it) || !(*it)->isActive()) {
                if (*it) {
                    (*it)->stop();
                }
                it = connection_pool_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

} // namespace simple_sftpd
