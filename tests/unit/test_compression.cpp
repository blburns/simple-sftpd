/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <gtest/gtest.h>
#include "simple-sftpd/utils/compression.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include <cstring>
#include <vector>

using namespace simple_sftpd;

class CompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_shared<Logger>("", LogLevel::WARN, false, false, LogFormat::STANDARD);
        compression_ = std::make_shared<Compression>(logger_);
    }
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Compression> compression_;
};

TEST_F(CompressionTest, IsSupportedNone) {
    EXPECT_TRUE(compression_->isSupported(Compression::Type::NONE));
}

TEST_F(CompressionTest, CompressNoneReturnsSame) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto out = compression_->compress(data, Compression::Type::NONE);
    EXPECT_EQ(out.size(), data.size());
    EXPECT_EQ(out, data);
}

TEST_F(CompressionTest, DecompressNoneReturnsSame) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto out = compression_->decompress(data, Compression::Type::NONE);
    EXPECT_EQ(out.size(), data.size());
    EXPECT_EQ(out, data);
}

#ifdef ENABLE_COMPRESSION
TEST_F(CompressionTest, GzipCompressDecompressRoundTrip) {
    std::string plain = "Hello, simple-sftpd compression test.";
    std::vector<uint8_t> data(plain.begin(), plain.end());
    auto compressed = compression_->compress(data, Compression::Type::GZIP);
    EXPECT_FALSE(compressed.empty());
    EXPECT_TRUE(compression_->isSupported(Compression::Type::GZIP));
    auto decompressed = compression_->decompress(compressed, Compression::Type::GZIP);
    std::string result(decompressed.begin(), decompressed.end());
    EXPECT_EQ(result, plain);
}

TEST_F(CompressionTest, ZlibStreamRoundTrip) {
    std::string plain = "MODE Z streaming zlib round-trip payload for simple-sftpd.";
    ZlibStream deflate(ZlibStream::Mode::Deflate);
    ZlibStream inflate(ZlibStream::Mode::Inflate);
    ASSERT_TRUE(deflate.valid());
    ASSERT_TRUE(inflate.valid());
    std::vector<uint8_t> compressed;
    ASSERT_TRUE(deflate.process(reinterpret_cast<const uint8_t*>(plain.data()), plain.size(),
                                compressed, false));
    std::vector<uint8_t> tail;
    ASSERT_TRUE(deflate.process(nullptr, 0, tail, true));
    compressed.insert(compressed.end(), tail.begin(), tail.end());
    EXPECT_FALSE(compressed.empty());
    std::vector<uint8_t> out;
    ASSERT_TRUE(inflate.process(compressed.data(), compressed.size(), out, true));
    std::string result(out.begin(), out.end());
    EXPECT_EQ(result, plain);
}
#endif

TEST_F(CompressionTest, EmptyInputCompress) {
    std::vector<uint8_t> empty;
    auto out = compression_->compress(empty, Compression::Type::NONE);
    EXPECT_TRUE(out.empty());
}
