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
#endif

TEST_F(CompressionTest, EmptyInputCompress) {
    std::vector<uint8_t> empty;
    auto out = compression_->compress(empty, Compression::Type::NONE);
    EXPECT_TRUE(out.empty());
}
