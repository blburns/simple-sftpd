/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <gtest/gtest.h>
#include "simple-sftpd/core/server.hpp"
#include "simple-sftpd/config/server_config.hpp"
#include "simple-sftpd/utils/compression.hpp"
#include "simple-sftpd/virtual_host/virtual_host.hpp"
#include "simple-sftpd/virtual_host/virtual_host_manager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#ifdef SIMPLE_SFTPD_SSL_ENABLED
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#ifdef HAVE_PAM
#include "simple-sftpd/security/pam_auth.hpp"
#endif

using namespace simple_sftpd;

namespace {

int connectTcp(const std::string& ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool sendAll(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

std::string recvLine(int fd) {
    std::string line;
    char c = 0;
    while (recv(fd, &c, 1, 0) == 1) {
        if (c == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        line.push_back(c);
        if (line.size() > 4096) {
            break;
        }
    }
    return line;
}

std::string readUntilFinal(int fd) {
    std::string last;
    for (int i = 0; i < 32; ++i) {
        last = recvLine(fd);
        if (last.size() >= 4 && last[3] == ' ') {
            return last;
        }
        if (last.empty()) {
            return last;
        }
    }
    return last;
}

bool parsePasv(const std::string& line, std::string& ip, int& port) {
    auto l = line.find('(');
    auto r = line.find(')');
    if (l == std::string::npos || r == std::string::npos || r <= l) {
        return false;
    }
    std::string inner = line.substr(l + 1, r - l - 1);
    std::vector<int> parts;
    std::istringstream iss(inner);
    std::string tok;
    while (std::getline(iss, tok, ',')) {
        try {
            parts.push_back(std::stoi(tok));
        } catch (...) {
            return false;
        }
    }
    if (parts.size() != 6) {
        return false;
    }
    ip = std::to_string(parts[0]) + "." + std::to_string(parts[1]) + "." +
         std::to_string(parts[2]) + "." + std::to_string(parts[3]);
    port = parts[4] * 256 + parts[5];
    return true;
}

std::string recvAll(int fd) {
    std::string out;
    char buf[4096];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        out.append(buf, static_cast<size_t>(n));
    }
    return out;
}

} // namespace

class FtpProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        home_ = std::filesystem::path("/tmp") /
                ("sftpd-itest-" + std::to_string(getpid()) + "-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(home_);
        hello_path_ = home_ / "hello.txt";
        {
            std::ofstream f(hello_path_);
            f << "hello-sftpd";
        }

        config_ = std::make_shared<FTPServerConfig>();
        config_->connection.bind_address = "127.0.0.1";
        config_->connection.bind_port = 0;
        config_->connection.passive_port_range_start = 49152;
        config_->connection.passive_port_range_end = 49252;
        config_->transfer.enable_compression = true;
        config_->transfer.use_sendfile = false;
        config_->transfer.use_mmap = false;
        config_->logging.log_to_console = false;
        config_->logging.log_level = "ERROR";

        server_ = std::make_shared<FTPServer>(config_);
        ASSERT_TRUE(server_->start());
        port_ = server_->listenPort();
        ASSERT_GT(port_, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override {
        if (ctrl_ >= 0) {
            close(ctrl_);
            ctrl_ = -1;
        }
        if (server_) {
            server_->stop();
        }
        std::error_code ec;
        std::filesystem::remove_all(home_, ec);
    }

    void connectControl() {
        ctrl_ = connectTcp("127.0.0.1", port_);
        ASSERT_GE(ctrl_, 0);
        std::string welcome = recvLine(ctrl_);
        ASSERT_EQ(welcome.substr(0, 3), "220");
    }

    std::string cmd(const std::string& c) {
        EXPECT_TRUE(sendAll(ctrl_, c + "\r\n"));
        return readUntilFinal(ctrl_);
    }

    bool login() {
        auto u = cmd("USER test");
        if (u.substr(0, 3) != "331") {
            ADD_FAILURE() << "USER failed: [" << u << "]";
            return false;
        }
        auto p = cmd("PASS test");
        if (p.substr(0, 3) != "230") {
            ADD_FAILURE() << "PASS failed: [" << p << "]";
            return false;
        }
        auto cwd = cmd("CWD " + home_.filename().string());
        if (cwd.substr(0, 3) != "250") {
            ADD_FAILURE() << "CWD failed: [" << cwd << "]";
            return false;
        }
        return true;
    }

    std::shared_ptr<FTPServerConfig> config_;
    std::shared_ptr<FTPServer> server_;
    std::filesystem::path home_;
    std::filesystem::path hello_path_;
    int port_ = 0;
    int ctrl_ = -1;
};

TEST_F(FtpProtocolTest, PasvRetrStor) {
    connectControl();
    ASSERT_TRUE(login());

    auto pasv = cmd("PASV");
    ASSERT_EQ(pasv.substr(0, 3), "227");
    std::string ip;
    int dport = 0;
    ASSERT_TRUE(parsePasv(pasv, ip, dport));
    int data = connectTcp("127.0.0.1", dport);
    ASSERT_GE(data, 0);
    EXPECT_TRUE(sendAll(ctrl_, "RETR hello.txt\r\n"));
    auto opening = recvLine(ctrl_);
    ASSERT_EQ(opening.substr(0, 3), "150");
    std::string body = recvAll(data);
    close(data);
    auto done = recvLine(ctrl_);
    ASSERT_EQ(done.substr(0, 3), "226");
    EXPECT_EQ(body, "hello-sftpd");

    auto pasv2 = cmd("PASV");
    ASSERT_EQ(pasv2.substr(0, 3), "227");
    ASSERT_TRUE(parsePasv(pasv2, ip, dport));
    int data2 = connectTcp("127.0.0.1", dport);
    ASSERT_GE(data2, 0);
    EXPECT_TRUE(sendAll(ctrl_, "STOR upload.txt\r\n"));
    auto opening2 = recvLine(ctrl_);
    ASSERT_EQ(opening2.substr(0, 3), "150");
    EXPECT_TRUE(sendAll(data2, "uploaded-bytes"));
    close(data2);
    auto done2 = recvLine(ctrl_);
    ASSERT_EQ(done2.substr(0, 3), "226");
    std::ifstream in(home_ / "upload.txt");
    std::string got((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(got, "uploaded-bytes");
}

TEST_F(FtpProtocolTest, PortRetr) {
    connectControl();
    ASSERT_TRUE(login());

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(listen_fd, 0);
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    ASSERT_EQ(bind(listen_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)), 0);
    ASSERT_EQ(listen(listen_fd, 1), 0);
    socklen_t alen = sizeof(addr);
    ASSERT_EQ(getsockname(listen_fd, reinterpret_cast<struct sockaddr*>(&addr), &alen), 0);
    int lport = ntohs(addr.sin_port);
    int p1 = lport / 256;
    int p2 = lport % 256;
    auto port_resp = cmd("PORT 127,0,0,1," + std::to_string(p1) + "," + std::to_string(p2));
    ASSERT_EQ(port_resp.substr(0, 3), "200");

    EXPECT_TRUE(sendAll(ctrl_, "RETR hello.txt\r\n"));
    auto opening = recvLine(ctrl_);
    ASSERT_EQ(opening.substr(0, 3), "150");

    struct sockaddr_in peer{};
    socklen_t plen = sizeof(peer);
    int data = accept(listen_fd, reinterpret_cast<struct sockaddr*>(&peer), &plen);
    ASSERT_GE(data, 0);
    std::string body = recvAll(data);
    close(data);
    close(listen_fd);
    auto done = recvLine(ctrl_);
    ASSERT_EQ(done.substr(0, 3), "226");
    EXPECT_EQ(body, "hello-sftpd");
}

TEST_F(FtpProtocolTest, HostCommand) {
    const std::string hostname = "sftpd-itest-" + std::to_string(getpid()) + ".local";
    auto host = std::make_shared<FTPVirtualHost>(hostname, home_.string());
    auto* mgr = server_->virtualHostManager().get();
    ASSERT_NE(mgr, nullptr);
    if (!mgr->getVirtualHost(hostname)) {
        ASSERT_TRUE(mgr->addVirtualHost(host));
    }
    connectControl();
    auto resp = cmd("HOST " + hostname);
    EXPECT_EQ(resp.substr(0, 3), "220") << resp;
    auto missing = cmd("HOST no-such-host.example");
    EXPECT_EQ(missing.substr(0, 3), "550") << missing;
}

TEST_F(FtpProtocolTest, ModeZRetrStor) {
#ifdef ENABLE_COMPRESSION
    connectControl();
    ASSERT_TRUE(login());
    auto feat = cmd("FEAT");
    EXPECT_EQ(feat.substr(0, 3), "211");

    auto mode = cmd("MODE Z");
    ASSERT_EQ(mode.substr(0, 3), "200");

    auto pasv = cmd("PASV");
    ASSERT_EQ(pasv.substr(0, 3), "227");
    std::string ip;
    int dport = 0;
    ASSERT_TRUE(parsePasv(pasv, ip, dport));
    int data = connectTcp("127.0.0.1", dport);
    ASSERT_GE(data, 0);
    EXPECT_TRUE(sendAll(ctrl_, "RETR hello.txt\r\n"));
    auto opening = recvLine(ctrl_);
    ASSERT_EQ(opening.substr(0, 3), "150");
    std::string compressed = recvAll(data);
    close(data);
    auto done = recvLine(ctrl_);
    ASSERT_EQ(done.substr(0, 3), "226");
    ASSERT_FALSE(compressed.empty());

    ZlibStream inflate(ZlibStream::Mode::Inflate);
    ASSERT_TRUE(inflate.valid());
    std::vector<uint8_t> plain;
    ASSERT_TRUE(inflate.process(reinterpret_cast<const uint8_t*>(compressed.data()),
                                compressed.size(), plain, true));
    std::string result(plain.begin(), plain.end());
    EXPECT_EQ(result, "hello-sftpd");

    auto rest = cmd("REST 1");
    EXPECT_EQ(rest.substr(0, 3), "550");

    std::string payload = "compressed-upload";
    ZlibStream deflate(ZlibStream::Mode::Deflate);
    ASSERT_TRUE(deflate.valid());
    std::vector<uint8_t> c1, c2;
    ASSERT_TRUE(deflate.process(reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), c1, false));
    ASSERT_TRUE(deflate.process(nullptr, 0, c2, true));
    c1.insert(c1.end(), c2.begin(), c2.end());

    auto pasv2 = cmd("PASV");
    ASSERT_TRUE(parsePasv(pasv2, ip, dport));
    int data2 = connectTcp("127.0.0.1", dport);
    ASSERT_GE(data2, 0);
    EXPECT_TRUE(sendAll(ctrl_, "STOR zupload.txt\r\n"));
    auto opening2 = recvLine(ctrl_);
    ASSERT_EQ(opening2.substr(0, 3), "150");
    EXPECT_TRUE(sendAll(data2, std::string(reinterpret_cast<const char*>(c1.data()), c1.size())));
    close(data2);
    auto done2 = recvLine(ctrl_);
    ASSERT_EQ(done2.substr(0, 3), "226");
    std::ifstream in(home_ / "zupload.txt");
    std::string got((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(got, payload);
#else
    GTEST_SKIP() << "ENABLE_COMPRESSION not set";
#endif
}

TEST_F(FtpProtocolTest, AuthTlsSmoke) {
#ifdef SIMPLE_SFTPD_SSL_ENABLED
    auto cert_dir = home_ / "tls";
    std::filesystem::create_directories(cert_dir);
    auto cert = cert_dir / "server.crt";
    auto key = cert_dir / "server.key";
    std::string gen = "openssl req -x509 -newkey rsa:2048 -nodes -keyout '" + key.string() +
                      "' -out '" + cert.string() +
                      "' -days 1 -subj '/CN=localhost' >/dev/null 2>&1";
    if (std::system(gen.c_str()) != 0) {
        GTEST_SKIP() << "openssl not available to generate a test certificate";
    }

    server_->stop();
    config_->security.ssl_cert_file = cert.string();
    config_->security.ssl_key_file = key.string();
    server_ = std::make_shared<FTPServer>(config_);
    ASSERT_TRUE(server_->start());
    port_ = server_->listenPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    connectControl();
    auto feat = cmd("FEAT");
    EXPECT_NE(feat.find("211"), std::string::npos);
    EXPECT_TRUE(sendAll(ctrl_, "AUTH TLS\r\n"));
    auto auth = recvLine(ctrl_);
    ASSERT_EQ(auth.substr(0, 3), "234");

    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    ASSERT_NE(ctx, nullptr);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL* ssl = SSL_new(ctx);
    ASSERT_NE(ssl, nullptr);
    SSL_set_fd(ssl, ctrl_);
    ASSERT_EQ(SSL_connect(ssl), 1);

    const char* user = "USER test\r\n";
    SSL_write(ssl, user, static_cast<int>(std::strlen(user)));
    char buf[256];
    int n = SSL_read(ssl, buf, sizeof(buf) - 1);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    EXPECT_NE(std::string(buf).find("331"), std::string::npos);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
#else
    GTEST_SKIP() << "SSL not enabled in this build";
#endif
}

TEST(PamAuthTest, SkippedUnlessLinuxPam) {
#ifdef HAVE_PAM
    auto logger = std::make_shared<Logger>("", LogLevel::ERROR, false, false, LogFormat::STANDARD);
    PAMAuth pam(logger);
    if (!pam.isAvailable()) {
        GTEST_SKIP() << "PAM library present but not usable";
    }
    EXPECT_FALSE(pam.authenticate("definitely-not-a-real-sftpd-user", "bad-password"));
#else
    GTEST_SKIP() << "PAM not available on this platform";
#endif
}
