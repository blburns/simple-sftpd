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
#include <map>
#include <mutex>
#include <chrono>
#include <memory>
#include <atomic>

namespace simple_sftpd {

class Logger;

/**
 * @brief File Metadata Cache
 * 
 * Caches file metadata to reduce filesystem operations
 */
class FileCache {
public:
    struct FileMetadata {
        std::string path;
        size_t size;
        bool is_directory;
        std::chrono::system_clock::time_point last_modified;
        std::chrono::system_clock::time_point cache_time;
    };

    FileCache(std::shared_ptr<Logger> logger, size_t max_entries = 1000, 
              std::chrono::seconds ttl = std::chrono::seconds(60));
    ~FileCache() = default;

    /**
     * @brief Get cached metadata
     * @param path File path
     * @return Metadata pointer if found and valid, nullptr otherwise
     */
    std::shared_ptr<FileMetadata> get(const std::string& path);

    /**
     * @brief Store metadata in cache
     * @param path File path
     * @param metadata Metadata to cache
     */
    void put(const std::string& path, const FileMetadata& metadata);

    /**
     * @brief Invalidate cache entry
     * @param path File path
     */
    void invalidate(const std::string& path);

    /**
     * @brief Clear all cache entries
     */
    void clear();

    /**
     * @brief Get cache statistics
     */
    size_t getSize() const;
    size_t getHits() const { return cache_hits_; }
    size_t getMisses() const { return cache_misses_; }

private:
    std::shared_ptr<Logger> logger_;
    mutable std::mutex cache_mutex_;
    std::map<std::string, std::shared_ptr<FileMetadata>> cache_;
    size_t max_entries_;
    std::chrono::seconds ttl_;
    
    std::atomic<size_t> cache_hits_;
    std::atomic<size_t> cache_misses_;
    
    void evictOldEntries();
    bool isExpired(const FileMetadata& metadata) const;
};

} // namespace simple_sftpd

