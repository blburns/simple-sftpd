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
#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/user/user.hpp"
#include "simple-sftpd/utils/logger.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

using namespace simple_sftpd;

class FTPUserManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_shared<Logger>("", LogLevel::ERROR, false, false, LogFormat::STANDARD);
        manager_ = std::make_shared<FTPUserManager>(logger_);
    }

    std::shared_ptr<Logger> logger_;
    std::shared_ptr<FTPUserManager> manager_;
};

TEST_F(FTPUserManagerTest, AddUser) {
    auto user = std::make_shared<FTPUser>("testuser", "testpass", "/home/testuser");
    manager_->addUser(user);
    
    auto retrieved = manager_->getUser("testuser");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getUsername(), "testuser");
    EXPECT_EQ(retrieved->getHomeDirectory(), "/home/testuser");
}

TEST_F(FTPUserManagerTest, GetUserNotFound) {
    auto user = manager_->getUser("nonexistent");
    EXPECT_EQ(user, nullptr);
}

TEST_F(FTPUserManagerTest, GetUserAfterAdd) {
    auto user1 = std::make_shared<FTPUser>("user1", "pass1", "/home/user1");
    auto user2 = std::make_shared<FTPUser>("user2", "pass2", "/home/user2");
    
    manager_->addUser(user1);
    manager_->addUser(user2);
    
    auto retrieved1 = manager_->getUser("user1");
    auto retrieved2 = manager_->getUser("user2");
    
    ASSERT_NE(retrieved1, nullptr);
    ASSERT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved1->getUsername(), "user1");
    EXPECT_EQ(retrieved2->getUsername(), "user2");
}

TEST_F(FTPUserManagerTest, RemoveUser) {
    auto user = std::make_shared<FTPUser>("testuser", "testpass", "/home/testuser");
    manager_->addUser(user);
    
    EXPECT_NE(manager_->getUser("testuser"), nullptr);
    
    manager_->removeUser("testuser");
    
    EXPECT_EQ(manager_->getUser("testuser"), nullptr);
}

TEST_F(FTPUserManagerTest, RemoveUserNotFound) {
    // Should not throw or crash
    manager_->removeUser("nonexistent");
    SUCCEED();
}

TEST_F(FTPUserManagerTest, ListUsers) {
    manager_->addUser(std::make_shared<FTPUser>("user1", "pass1", "/home/user1"));
    manager_->addUser(std::make_shared<FTPUser>("user2", "pass2", "/home/user2"));
    manager_->addUser(std::make_shared<FTPUser>("user3", "pass3", "/home/user3"));
    
    auto usernames = manager_->listUsers();
    EXPECT_EQ(usernames.size(), 3U);
}

TEST_F(FTPUserManagerTest, UserCount) {
    auto usernames = manager_->listUsers();
    EXPECT_EQ(usernames.size(), 0U);
    
    manager_->addUser(std::make_shared<FTPUser>("user1", "pass1", "/home/user1"));
    usernames = manager_->listUsers();
    EXPECT_EQ(usernames.size(), 1U);
    
    manager_->addUser(std::make_shared<FTPUser>("user2", "pass2", "/home/user2"));
    usernames = manager_->listUsers();
    EXPECT_EQ(usernames.size(), 2U);
    
    manager_->removeUser("user1");
    usernames = manager_->listUsers();
    EXPECT_EQ(usernames.size(), 1U);
}

#if defined(ENABLE_JSON)
TEST_F(FTPUserManagerTest, LegacyPlaintextFileIsMigratedOnLoad) {
    const auto dir = std::filesystem::temp_directory_path() /
                     ("sftpd-usertest-" + std::to_string(::getpid()));
    std::filesystem::create_directories(dir);
    const auto user_file = dir / "users.json";

    {
        std::ofstream out(user_file);
        out << R"({"users":[{"username":"legacy","password":"plaintext-secret",)"
               R"("home_directory":"/tmp","permissions":["read","write:/uploads"]}],)"
               R"("version":"1.0"})";
    }

    auto manager = std::make_shared<FTPUserManager>(logger_, user_file.string());
    auto user = manager->getUser("legacy");
    ASSERT_NE(user, nullptr);

    // The password still works, but is no longer stored in the clear.
    EXPECT_TRUE(user->authenticate("plaintext-secret"));
    EXPECT_FALSE(user->authenticate("wrong"));
    EXPECT_FALSE(user->hasLegacyPassword());

    // Permissions survive the round trip.
    ASSERT_EQ(user->getPermissions().size(), 2u);
    EXPECT_TRUE(user->hasPermission("write", "/tmp/uploads/x"));
    EXPECT_FALSE(user->hasPermission("write", "/tmp/elsewhere/x"));

    // And the file on disk was rewritten, so the plaintext is gone.
    std::ifstream in(user_file);
    const std::string contents((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("plaintext-secret"), std::string::npos);
    EXPECT_NE(contents.find("$pbkdf2-sha256$"), std::string::npos);

    // Reloading the migrated file must not double-hash.
    auto reloaded = std::make_shared<FTPUserManager>(logger_, user_file.string());
    auto reloaded_user = reloaded->getUser("legacy");
    ASSERT_NE(reloaded_user, nullptr);
    EXPECT_TRUE(reloaded_user->authenticate("plaintext-secret"));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
#endif

