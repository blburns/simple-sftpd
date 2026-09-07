/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <gtest/gtest.h>
#include "simple-sftpd/security/password.hpp"

#include <set>
#include <string>

using namespace simple_sftpd;

TEST(PasswordHashTest, HashIsSaltedAndSelfDescribing) {
    const std::string hashed = password::hash("correct horse battery staple");
    ASSERT_FALSE(hashed.empty());
    EXPECT_EQ(hashed.rfind("$pbkdf2-sha256$", 0), 0u);
    EXPECT_NE(hashed.find("$" + std::to_string(password::kDefaultIterations) + "$"),
              std::string::npos);
    EXPECT_EQ(hashed.find("correct horse"), std::string::npos);
}

TEST(PasswordHashTest, SameInputProducesDifferentHashes) {
    std::set<std::string> hashes;
    for (int i = 0; i < 5; ++i) {
        hashes.insert(password::hash("repeated"));
    }
    EXPECT_EQ(hashes.size(), 5u) << "salt is not random per hash";
}

TEST(PasswordHashTest, VerifyAcceptsOnlyTheRightPassword) {
    const std::string hashed = password::hash("s3cret");
    ASSERT_FALSE(hashed.empty());
    EXPECT_TRUE(password::verify("s3cret", hashed));
    EXPECT_FALSE(password::verify("s3cre", hashed));
    EXPECT_FALSE(password::verify("s3crett", hashed));
    EXPECT_FALSE(password::verify("S3cret", hashed));
    EXPECT_FALSE(password::verify("", hashed));
}

TEST(PasswordHashTest, EmptyPasswordRoundTrips) {
    const std::string hashed = password::hash("");
    ASSERT_FALSE(hashed.empty());
    EXPECT_TRUE(password::verify("", hashed));
    EXPECT_FALSE(password::verify("x", hashed));
}

TEST(PasswordHashTest, LongAndBinaryPasswordsRoundTrip) {
    const std::string long_password(4096, 'a');
    const std::string hashed = password::hash(long_password);
    ASSERT_FALSE(hashed.empty());
    EXPECT_TRUE(password::verify(long_password, hashed));

    std::string binary("bin\0ary", 7);
    const std::string binary_hash = password::hash(binary);
    ASSERT_FALSE(binary_hash.empty());
    EXPECT_TRUE(password::verify(binary, binary_hash));
    EXPECT_FALSE(password::verify("bin", binary_hash));
}

TEST(PasswordHashTest, IterationCountIsHonoredAndPortable) {
    const std::string hashed = password::hash("tuned", 1000);
    ASSERT_FALSE(hashed.empty());
    EXPECT_NE(hashed.find("$1000$"), std::string::npos);
    EXPECT_TRUE(password::verify("tuned", hashed));
}

TEST(PasswordHashTest, LegacyPlaintextStillVerifies) {
    // Existing user files hold plaintext until the first load migrates them.
    EXPECT_FALSE(password::isHashed("plaintext"));
    EXPECT_TRUE(password::verify("plaintext", "plaintext"));
    EXPECT_FALSE(password::verify("other", "plaintext"));
}

TEST(PasswordHashTest, MalformedHashesAreRejectedNotAccepted) {
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$"));
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$210000$"));
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$210000$bm90YmFzZTY0$"));
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$0$AAAA$AAAA"));
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$notanumber$AAAA$AAAA"));
    // Digest of the wrong length must not be accepted.
    EXPECT_FALSE(password::verify("x", "$pbkdf2-sha256$1000$AAAAAAAAAAAAAAAAAAAAAA==$AAAA"));
}

TEST(PasswordHashTest, ConstantTimeEqualsMatchesStringEquality) {
    EXPECT_TRUE(password::constantTimeEquals("", ""));
    EXPECT_TRUE(password::constantTimeEquals("abc", "abc"));
    EXPECT_FALSE(password::constantTimeEquals("abc", "abd"));
    EXPECT_FALSE(password::constantTimeEquals("abc", "ab"));
    EXPECT_FALSE(password::constantTimeEquals("ab", "abc"));
    EXPECT_FALSE(password::constantTimeEquals("", "a"));
}
