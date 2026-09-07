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
#include <vector>
#include <memory>
#include <cstddef>
#include <cstdint>

namespace simple_sftpd {

class Logger;

/**
 * @brief Compression Support
 * 
 * Provides compression/decompression for file transfers
 */
class Compression {
public:
    enum class Type {
        NONE,
        GZIP,
        BZIP2
    };

    Compression(std::shared_ptr<Logger> logger);
    ~Compression() = default;

    /**
     * @brief Compress data
     * @param data Input data
     * @param type Compression type
     * @return Compressed data
     */
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data, Type type);

    /**
     * @brief Decompress data
     * @param data Compressed data
     * @param type Compression type
     * @return Decompressed data
     */
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data, Type type);

    /**
     * @brief Check if compression type is supported
     * @param type Compression type
     * @return true if supported
     */
    bool isSupported(Type type) const;

private:
    std::shared_ptr<Logger> logger_;
    
    std::vector<uint8_t> compressGzip(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressGzip(const std::vector<uint8_t>& data);
    std::vector<uint8_t> compressBzip2(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressBzip2(const std::vector<uint8_t>& data);
};

/**
 * Streaming zlib (RFC 1950) for FTP MODE Z. MODE Z uses the zlib wrapper, not gzip.
 */
class ZlibStream {
public:
    enum class Mode {
        Deflate,
        Inflate
    };

    explicit ZlibStream(Mode mode);
    ~ZlibStream();

    ZlibStream(const ZlibStream&) = delete;
    ZlibStream& operator=(const ZlibStream&) = delete;

    bool valid() const;
    /**
     * Consume input and append compressed/decompressed bytes to output.
     * Pass finish=true after the last input chunk (input may be empty).
     */
    bool process(const uint8_t* input, size_t input_len, std::vector<uint8_t>& output, bool finish);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace simple_sftpd

