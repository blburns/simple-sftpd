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

#include "simple-sftpd/utils/compression.hpp"
#include "simple-sftpd/utils/logger.hpp"
#ifdef ENABLE_COMPRESSION
#include <zlib.h>
#ifdef HAVE_BZIP2
#include <bzlib.h>
#endif
#endif
#include <cstring>
#include <memory>

namespace simple_sftpd {

Compression::Compression(std::shared_ptr<Logger> logger)
    : logger_(logger) {
}

std::vector<uint8_t> Compression::compress(const std::vector<uint8_t>& data, Type type) {
    switch (type) {
        case Type::GZIP:
            return compressGzip(data);
        case Type::BZIP2:
            return compressBzip2(data);
        case Type::NONE:
        default:
            return data;
    }
}

std::vector<uint8_t> Compression::decompress(const std::vector<uint8_t>& data, Type type) {
    switch (type) {
        case Type::GZIP:
            return decompressGzip(data);
        case Type::BZIP2:
            return decompressBzip2(data);
        case Type::NONE:
        default:
            return data;
    }
}

bool Compression::isSupported(Type type) const {
    switch (type) {
        case Type::GZIP:
#ifdef ENABLE_COMPRESSION
            return true;
#else
            return false;
#endif
        case Type::BZIP2:
#if defined(ENABLE_COMPRESSION) && defined(HAVE_BZIP2)
            return true;
#else
            return false;
#endif
        case Type::NONE:
        default:
            return true;
    }
}

std::vector<uint8_t> Compression::compressGzip(const std::vector<uint8_t>& data) {
#ifdef ENABLE_COMPRESSION
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        logger_->error("Failed to initialize gzip compression");
        return data;
    }
    
    zs.next_in = const_cast<Bytef*>(data.data());
    zs.avail_in = data.size();
    
    std::vector<uint8_t> output;
    output.resize(data.size() * 1.1 + 12); // Estimate compressed size
    zs.next_out = output.data();
    zs.avail_out = output.size();
    
    int ret = deflate(&zs, Z_FINISH);
    if (ret == Z_STREAM_END) {
        output.resize(zs.total_out);
    } else {
        logger_->error("Gzip compression failed");
        deflateEnd(&zs);
        return data;
    }
    
    deflateEnd(&zs);
    return output;
#else
    logger_->warn("Gzip compression not available - compression support disabled");
    return data;
#endif
}

std::vector<uint8_t> Compression::decompressGzip(const std::vector<uint8_t>& data) {
#ifdef ENABLE_COMPRESSION
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    
    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK) {
        logger_->error("Failed to initialize gzip decompression");
        return data;
    }
    
    zs.next_in = const_cast<Bytef*>(data.data());
    zs.avail_in = data.size();
    
    std::vector<uint8_t> output;
    output.resize(data.size() * 4); // Estimate decompressed size
    zs.next_out = output.data();
    zs.avail_out = output.size();
    
    int ret = inflate(&zs, Z_FINISH);
    if (ret == Z_STREAM_END) {
        output.resize(zs.total_out);
    } else {
        // Try with larger buffer
        output.resize(output.size() * 2);
        zs.next_out = output.data() + zs.total_out;
        zs.avail_out = output.size() - zs.total_out;
        ret = inflate(&zs, Z_FINISH);
        if (ret == Z_STREAM_END) {
            output.resize(zs.total_out);
        } else {
            logger_->error("Gzip decompression failed");
            inflateEnd(&zs);
            return data;
        }
    }
    
    inflateEnd(&zs);
    return output;
#else
    logger_->warn("Gzip decompression not available - compression support disabled");
    return data;
#endif
}

std::vector<uint8_t> Compression::compressBzip2(const std::vector<uint8_t>& data) {
#if defined(ENABLE_COMPRESSION) && defined(HAVE_BZIP2)
    unsigned int dest_len = data.size() * 1.1 + 600; // Estimate
    std::vector<uint8_t> output(dest_len);
    
    int ret = BZ2_bzBuffToBuffCompress(
        reinterpret_cast<char*>(output.data()), &dest_len,
        const_cast<char*>(reinterpret_cast<const char*>(data.data())), data.size(),
        9, 0, 0);
    
    if (ret == BZ_OK) {
        output.resize(dest_len);
        return output;
    } else {
        logger_->error("Bzip2 compression failed");
        return data;
    }
#else
    logger_->warn("Bzip2 compression not available - compression support disabled");
    return data;
#endif
}

std::vector<uint8_t> Compression::decompressBzip2(const std::vector<uint8_t>& data) {
#if defined(ENABLE_COMPRESSION) && defined(HAVE_BZIP2)
    unsigned int dest_len = data.size() * 4; // Estimate
    std::vector<uint8_t> output(dest_len);
    
    int ret = BZ2_bzBuffToBuffDecompress(
        reinterpret_cast<char*>(output.data()), &dest_len,
        const_cast<char*>(reinterpret_cast<const char*>(data.data())), data.size(),
        0, 0);
    
    if (ret == BZ_OK) {
        output.resize(dest_len);
        return output;
    } else {
        // Try with larger buffer
        dest_len = data.size() * 8;
        output.resize(dest_len);
        ret = BZ2_bzBuffToBuffDecompress(
            reinterpret_cast<char*>(output.data()), &dest_len,
            const_cast<char*>(reinterpret_cast<const char*>(data.data())), data.size(),
            0, 0);
        if (ret == BZ_OK) {
            output.resize(dest_len);
            return output;
        } else {
            logger_->error("Bzip2 decompression failed");
            return data;
        }
    }
#else
    logger_->warn("Bzip2 decompression not available - compression support disabled");
    return data;
#endif
}

struct ZlibStream::Impl {
#ifdef ENABLE_COMPRESSION
    z_stream stream{};
#endif
    ZlibStream::Mode mode = ZlibStream::Mode::Deflate;
    bool valid = false;
};

ZlibStream::ZlibStream(Mode mode) : impl_(std::make_unique<Impl>()) {
    impl_->mode = mode;
#ifdef ENABLE_COMPRESSION
    std::memset(&impl_->stream, 0, sizeof(impl_->stream));
    int ret = (mode == Mode::Deflate)
        ? deflateInit(&impl_->stream, Z_DEFAULT_COMPRESSION)
        : inflateInit(&impl_->stream);
    impl_->valid = (ret == Z_OK);
#else
    (void)mode;
    impl_->valid = false;
#endif
}

ZlibStream::~ZlibStream() {
#ifdef ENABLE_COMPRESSION
    if (impl_ && impl_->valid) {
        if (impl_->mode == Mode::Deflate) {
            deflateEnd(&impl_->stream);
        } else {
            inflateEnd(&impl_->stream);
        }
    }
#endif
}

bool ZlibStream::valid() const {
    return impl_ && impl_->valid;
}

bool ZlibStream::process(const uint8_t* input, size_t input_len, std::vector<uint8_t>& output, bool finish) {
    output.clear();
#ifdef ENABLE_COMPRESSION
    if (!impl_ || !impl_->valid) {
        return false;
    }
    static const uint8_t kEmpty = 0;
    impl_->stream.next_in = const_cast<Bytef*>(input != nullptr ? input : &kEmpty);
    impl_->stream.avail_in = static_cast<uInt>(input_len);
    const int flush = finish ? Z_FINISH : Z_NO_FLUSH;
    uint8_t buf[16384];
    int ret = Z_OK;
    do {
        impl_->stream.next_out = buf;
        impl_->stream.avail_out = sizeof(buf);
        ret = (impl_->mode == Mode::Deflate)
            ? deflate(&impl_->stream, flush)
            : inflate(&impl_->stream, flush);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            return false;
        }
        const size_t have = sizeof(buf) - impl_->stream.avail_out;
        output.insert(output.end(), buf, buf + have);
        if (ret == Z_STREAM_END) {
            break;
        }
        if (ret == Z_BUF_ERROR && impl_->stream.avail_in == 0 && !finish) {
            break;
        }
    } while (impl_->stream.avail_out == 0 || (finish && ret != Z_STREAM_END));
    return ret == Z_OK || ret == Z_STREAM_END || ret == Z_BUF_ERROR;
#else
    (void)input;
    (void)input_len;
    (void)finish;
    return false;
#endif
}

} // namespace simple_sftpd

