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

#include <gtest/gtest.h>
#include "simple-sftpd/user/user.hpp"

using namespace simple_sftpd;

class FTPUserTest : public ::testing::Test {
protected:
    void SetUp() override {
        user_ = std::make_shared<FTPUser>("testuser", "testpass", "/home/testuser");
    }

    std::shared_ptr<FTPUser> user_;
};

TEST_F(FTPUserTest, Constructor) {
    EXPECT_EQ(user_->getUsername(), "testuser");
    EXPECT_EQ(user_->getHomeDirectory(), "/home/testuser");
}

TEST_F(FTPUserTest, ConstructorHashesPlaintext) {
    EXPECT_NE(user_->getPasswordHash(), "testpass");
    EXPECT_FALSE(user_->hasLegacyPassword());
    EXPECT_EQ(user_->getPasswordHash().rfind("$pbkdf2-sha256$", 0), 0u);
}

TEST_F(FTPUserTest, EachUserGetsADistinctSalt) {
    FTPUser other("otheruser", "testpass", "/home/otheruser");
    EXPECT_NE(user_->getPasswordHash(), other.getPasswordHash());
    EXPECT_TRUE(other.authenticate("testpass"));
}

TEST_F(FTPUserTest, AnAlreadyHashedCredentialIsStoredVerbatim) {
    const std::string existing = user_->getPasswordHash();
    FTPUser reloaded("testuser", existing, "/home/testuser");
    EXPECT_EQ(reloaded.getPasswordHash(), existing);
    EXPECT_TRUE(reloaded.authenticate("testpass"));
}

TEST_F(FTPUserTest, AuthenticateSuccess) {
    EXPECT_TRUE(user_->authenticate("testpass"));
}

TEST_F(FTPUserTest, AuthenticateFailure) {
    EXPECT_FALSE(user_->authenticate("wrongpass"));
    EXPECT_FALSE(user_->authenticate(""));
    EXPECT_FALSE(user_->authenticate("testpass "));
}

TEST_F(FTPUserTest, SetUsername) {
    user_->setUsername("newuser");
    EXPECT_EQ(user_->getUsername(), "newuser");
}

TEST_F(FTPUserTest, SetPassword) {
    user_->setPassword("newpass");
    EXPECT_TRUE(user_->authenticate("newpass"));
    EXPECT_FALSE(user_->authenticate("testpass"));
}

TEST_F(FTPUserTest, SetHomeDirectory) {
    user_->setHomeDirectory("/home/newuser");
    EXPECT_EQ(user_->getHomeDirectory(), "/home/newuser");
}

TEST_F(FTPUserTest, HasPermissionDefault) {
    // Default behavior: empty permissions means allow all
    EXPECT_TRUE(user_->hasPermission("read", "/home/testuser/file.txt"));
    EXPECT_TRUE(user_->hasPermission("write", "/home/testuser/file.txt"));
    EXPECT_TRUE(user_->hasPermission("list", "/home/testuser"));
}

TEST_F(FTPUserTest, UnscopedPermissionsGateByOperation) {
    user_->setPermissions({"read", "list"});
    EXPECT_TRUE(user_->hasPermission("read", "/home/testuser/file.txt"));
    EXPECT_TRUE(user_->hasPermission("list", "/home/testuser"));
    EXPECT_FALSE(user_->hasPermission("write", "/home/testuser/file.txt"));
}

TEST_F(FTPUserTest, AllPermissionCoversEveryOperation) {
    user_->setPermissions({"all"});
    EXPECT_TRUE(user_->hasPermission("read", "/home/testuser/a"));
    EXPECT_TRUE(user_->hasPermission("write", "/home/testuser/a"));
}

TEST_F(FTPUserTest, ScopedPermissionsAreLimitedToTheirSubtree) {
    user_->setPermissions({"read", "write:/uploads"});

    EXPECT_TRUE(user_->hasPermission("write", "/home/testuser/uploads/report.csv"));
    EXPECT_TRUE(user_->hasPermission("write", "/home/testuser/uploads"));
    EXPECT_FALSE(user_->hasPermission("write", "/home/testuser/other/report.csv"));

    // A sibling directory sharing a name prefix must not match the scope.
    EXPECT_FALSE(user_->hasPermission("write", "/home/testuser/uploads-archive/x"));

    // Reads stay unscoped.
    EXPECT_TRUE(user_->hasPermission("read", "/home/testuser/other/report.csv"));
}

TEST_F(FTPUserTest, AddPermissionDeduplicates) {
    user_->addPermission("read");
    user_->addPermission("read");
    EXPECT_EQ(user_->getPermissions().size(), 1u);
}

TEST_F(FTPUserTest, DefaultConstructor) {
    FTPUser user;
    EXPECT_EQ(user.getUsername(), "");
    EXPECT_EQ(user.getHomeDirectory(), "");
    EXPECT_TRUE(user.hasPermission("read", "")); // Default allows all
}

