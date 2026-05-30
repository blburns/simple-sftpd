# Examples

This section provides practical examples and code samples for using Simple-Secure FTP Daemon in various scenarios.

## 🚀 Quick Start Examples

### Basic Server Setup

#### Minimal Server Configuration
```cpp
#include "simple-sftpd/ftp_server.hpp"
#include "simple-sftpd/ftp_server_config.hpp"

int main() {
    // Create minimal configuration
    auto config = std::make_shared<simple_sftpd::FTPServerConfig>();
    
    // Set basic settings
    config->connection.bind_address = "0.0.0.0";
    config->connection.bind_port = 21;
    config->connection.max_connections = 50;
    
    // Create and start server
    simple_sftpd::FTPServer server(config);
    if (server.start()) {
        std::cout << "FTP Server started on port 21" << std::endl;
        
        // Keep server running
        while (server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    return 0;
}
```

#### Server with Configuration File
```cpp
#include "simple-sftpd/ftp_server.hpp"
#include "simple-sftpd/ftp_server_config.hpp"
#include "simple-sftpd/logger.hpp"

int main(int argc, char* argv[]) {
    std::string config_file = "/etc/simple-sftpd/simple-sftpd.conf";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        }
    }
    
    // Create logger
    auto logger = std::make_shared<simple_sftpd::Logger>(
        "/var/log/simple-sftpd/simple-sftpd.log",
        simple_sftpd::LogLevel::INFO
    );
    
    // Load configuration
    auto config = std::make_shared<simple_sftpd::FTPServerConfig>();
    if (!config->loadFromFile(config_file)) {
        logger->error("Failed to load configuration from: " + config_file);
        return 1;
    }
    
    // Validate configuration
    if (!config->validate()) {
        auto errors = config->getErrors();
        for (const auto& error : errors) {
            logger->error("Configuration error: " + error);
        }
        return 1;
    }
    
    // Create and start server
    simple_sftpd::FTPServer server(config);
    if (server.start()) {
        logger->info("FTP Server started successfully");
        
        // Wait for shutdown signal
        while (server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        logger->info("FTP Server stopped");
    } else {
        logger->error("Failed to start FTP Server");
        return 1;
    }
    
    return 0;
}
```

## 👥 User Management Examples

### Creating and Managing Users

#### Basic User Creation
```cpp
#include "simple-sftpd/ftp_user.hpp"

void createBasicUser() {
    // Create user
    auto user = std::make_shared<simple_sftpd::FTPUser>("john");
    
    // Set password
    user->setPassword("secret123");
    
    // Set home directory
    user->setHomeDirectory("/var/ftp/john");
    
    // Grant basic permissions
    user->grantPermission(simple_sftpd::UserPermission::READ);
    user->grantPermission(simple_sftpd::UserPermission::LIST);
    user->grantPermission(simple_sftpd::UserPermission::DOWNLOAD);
    
    // Set user status
    user->setStatus(simple_sftpd::UserStatus::ACTIVE);
    
    // Add to server
    server.addUser(user);
}
```

#### Advanced User with All Permissions
```cpp
#include "simple-sftpd/ftp_user.hpp"

void createAdminUser() {
    auto admin = std::make_shared<simple_sftpd::FTPUser>("admin");
    
    // Set secure password
    admin->setPassword("SuperSecurePassword123!");
    
    // Set home directory
    admin->setHomeDirectory("/var/ftp/admin");
    
    // Grant all permissions
    std::vector<simple_sftpd::UserPermission> all_permissions = {
        simple_sftpd::UserPermission::READ,
        simple_sftpd::UserPermission::WRITE,
        simple_sftpd::UserPermission::DELETE,
        simple_sftpd::UserPermission::RENAME,
        simple_sftpd::UserPermission::MKDIR,
        simple_sftpd::UserPermission::RMDIR,
        simple_sftpd::UserPermission::LIST,
        simple_sftpd::UserPermission::UPLOAD,
        simple_sftpd::UserPermission::DOWNLOAD,
        simple_sftpd::UserPermission::APPEND,
        simple_sftpd::UserPermission::CHMOD,
        simple_sftpd::UserPermission::CHOWN,
        simple_sftpd::UserPermission::SYMLINK,
        simple_sftpd::UserPermission::EXECUTE,
        simple_sftpd::UserPermission::ADMIN
    };
    
    for (const auto& permission : all_permissions) {
        admin->grantPermission(permission);
    }
    
    // Set user properties
    admin->setStatus(simple_sftpd::UserStatus::ACTIVE);
    admin->setMaxConnections(10);
    admin->setTransferRateLimit(1024 * 1024); // 1 MB/s
    
    server.addUser(admin);
}
```

#### User with Restricted Access
```cpp
#include "simple-sftpd/ftp_user.hpp"

void createRestrictedUser() {
    auto guest = std::make_shared<simple_sftpd::FTPUser>("guest");
    
    // Set password
    guest->setPassword("guest123");
    
    // Set restricted home directory
    guest->setHomeDirectory("/var/ftp/guest");
    
    // Grant minimal permissions
    guest->grantPermission(simple_sftpd::UserPermission::READ);
    guest->grantPermission(simple_sftpd::UserPermission::LIST);
    guest->grantPermission(simple_sftpd::UserPermission::DOWNLOAD);
    
    // Set restrictions
    guest->setMaxConnections(1);
    guest->setTransferRateLimit(512 * 1024); // 512 KB/s
    guest->setMaxFileSize(10 * 1024 * 1024); // 10 MB max file size
    
    // Set user status
    guest->setStatus(simple_sftpd::UserStatus::ACTIVE);
    
    server.addUser(guest);
}
```

#### Anonymous User
```cpp
#include "simple-sftpd/ftp_user.hpp"

void createAnonymousUser() {
    auto anon = std::make_shared<simple_sftpd::FTPUser>("anonymous");
    
    // Set empty password for anonymous access
    anon->setPassword("");
    
    // Set public directory
    anon->setHomeDirectory("/var/ftp/public");
    
    // Grant read-only access
    anon->grantPermission(simple_sftpd::UserPermission::READ);
    anon->grantPermission(simple_sftpd::UserPermission::LIST);
    anon->grantPermission(simple_sftpd::UserPermission::DOWNLOAD);
    
    // Set restrictions
    anon->setMaxConnections(5);
    anon->setTransferRateLimit(256 * 1024); // 256 KB/s
    anon->setMaxFileSize(5 * 1024 * 1024); // 5 MB max file size
    
    // Set user status
    anon->setStatus(simple_sftpd::UserStatus::ACTIVE);
    
    server.addUser(anon);
}
```

## 🌐 Virtual Host Examples

### Basic Virtual Host Setup

#### Single Domain Virtual Host
```cpp
#include "simple-sftpd/ftp_virtual_host.hpp"

void createBasicVirtualHost() {
    auto vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("ftp.example.com");
    
    // Set document root
    vhost->setDocumentRoot("/var/ftp/example");
    
    // Set welcome messages
    vhost->setWelcomeMessage("Welcome to Example.com FTP Server");
    vhost->setBannerMessage("Example.com FTP Server Ready");
    
    // Configure basic security
    simple_sftpd::SecurityConfig security;
    security.chroot_enabled = true;
    security.chroot_directory = "/var/ftp/example";
    security.drop_privileges = true;
    security.run_as_user = "ftp";
    security.run_as_group = "ftp";
    
    vhost->setSecurityConfig(security);
    
    // Add to server
    server.addVirtualHost(vhost);
}
```

#### Multi-Domain Virtual Hosts
```cpp
#include "simple-sftpd/ftp_virtual_host.hpp"

void createMultiDomainSetup() {
    // Example.com virtual host
    auto example_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("ftp.example.com");
    example_vhost->setDocumentRoot("/var/ftp/example");
    example_vhost->setWelcomeMessage("Welcome to Example.com FTP");
    
    // Test.com virtual host
    auto test_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("ftp.test.com");
    test_vhost->setDocumentRoot("/var/ftp/test");
    test_vhost->setWelcomeMessage("Welcome to Test.com FTP");
    
    // Demo.com virtual host
    auto demo_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("ftp.demo.com");
    demo_vhost->setDocumentRoot("/var/ftp/demo");
    demo_vhost->setWelcomeMessage("Welcome to Demo.com FTP");
    
    // Add all virtual hosts
    server.addVirtualHost(example_vhost);
    server.addVirtualHost(test_vhost);
    server.addVirtualHost(demo_vhost);
}
```

### SSL/TLS Virtual Hosts

#### SSL-Enabled Virtual Host
```cpp
#include "simple-sftpd/ftp_virtual_host.hpp"

void createSSLVirtualHost() {
    auto ssl_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("secure.example.com");
    
    // Set document root
    ssl_vhost->setDocumentRoot("/var/ftp/secure");
    
    // Configure SSL
    simple_sftpd::SSLConfig ssl_config;
    ssl_config.enabled = true;
    ssl_config.certificate_file = "/etc/ssl/certs/example.com.crt";
    ssl_config.private_key_file = "/etc/ssl/private/example.com.key";
    ssl_config.ca_certificate_file = "/etc/ssl/certs/ca-bundle.crt";
    ssl_config.cipher_suite = "ECDHE-RSA-AES256-GCM-SHA384";
    ssl_config.require_client_cert = false;
    ssl_config.verify_peer = true;
    ssl_config.min_tls_version = 0x0303; // TLS 1.2
    ssl_config.max_tls_version = 0x0304; // TLS 1.3
    
    ssl_vhost->setSSLConfig(ssl_config);
    
    // Configure security
    simple_sftpd::SecurityConfig security;
    security.chroot_enabled = true;
    security.chroot_directory = "/var/ftp/secure";
    security.drop_privileges = true;
    security.run_as_user = "ftp";
    security.require_ssl = true;
    
    ssl_vhost->setSecurityConfig(security);
    
    // Set welcome message
    ssl_vhost->setWelcomeMessage("Welcome to Secure Example.com FTP (FTPS)");
    
    server.addVirtualHost(ssl_vhost);
}
```

#### Mixed HTTP/FTP Virtual Host
```cpp
#include "simple-sftpd/ftp_virtual_host.hpp"

void createMixedVirtualHost() {
    auto mixed_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("files.example.com");
    
    // Set document root
    mixed_vhost->setDocumentRoot("/var/www/example.com/files");
    
    // Configure SSL (optional for this host)
    simple_sftpd::SSLConfig ssl_config;
    ssl_config.enabled = true;
    ssl_config.certificate_file = "/etc/ssl/certs/example.com.crt";
    ssl_config.private_key_file = "/etc/ssl/private/example.com.key";
    ssl_config.require_client_cert = false;
    
    mixed_vhost->setSSLConfig(ssl_config);
    
    // Configure security
    simple_sftpd::SecurityConfig security;
    security.chroot_enabled = false; // Allow access to web files
    security.drop_privileges = true;
    security.run_as_user = "www-data";
    security.run_as_group = "www-data";
    security.require_ssl = false; // Allow both FTP and FTPS
    
    mixed_vhost->setSecurityConfig(security);
    
    // Set welcome message
    mixed_vhost->setWelcomeMessage("Welcome to Example.com File Server");
    
    server.addVirtualHost(mixed_vhost);
}
```

## 🔒 Security Examples

### Advanced Security Configuration

#### Chroot Jail Setup
```cpp
#include "simple-sftpd/ftp_virtual_host.hpp"

void createSecureChrootJail() {
    auto secure_vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("secure.example.com");
    
    // Set document root
    secure_vhost->setDocumentRoot("/var/ftp/secure");
    
    // Configure strict security
    simple_sftpd::SecurityConfig security;
    security.chroot_enabled = true;
    security.chroot_directory = "/var/ftp/secure";
    security.drop_privileges = true;
    security.run_as_user = "ftp";
    security.run_as_group = "ftp";
    security.allow_anonymous = false;
    security.allow_guest = false;
    security.require_ssl = true;
    
    // Restrict commands
    security.allowed_commands = {
        "USER", "PASS", "QUIT", "PWD", "CWD", "CDUP",
        "TYPE", "MODE", "STRU", "PASV", "PORT", "LIST",
        "NLST", "RETR", "STOR", "APPE", "MKD", "RMD",
        "DELE", "RNFR", "RNTO", "SIZE", "MDTM"
    };
    
    // Set timeouts
    security.login_timeout = std::chrono::seconds(30);
    security.session_timeout = std::chrono::seconds(1800);
    security.max_login_attempts = 3;
    
    secure_vhost->setSecurityConfig(security);
    
    // Configure SSL
    simple_sftpd::SSLConfig ssl_config;
    ssl_config.enabled = true;
    ssl_config.certificate_file = "/etc/ssl/certs/secure.example.com.crt";
    ssl_config.private_key_file = "/etc/ssl/private/secure.example.com.key";
    ssl_config.require_client_cert = true;
    ssl_config.verify_peer = true;
    
    secure_vhost->setSSLConfig(ssl_config);
    
    server.addVirtualHost(secure_vhost);
}
```

#### Rate Limiting and DDoS Protection
```cpp
#include "simple-sftpd/ftp_server_config.hpp"

void configureRateLimiting() {
    auto config = std::make_shared<simple_sftpd::FTPServerConfig>();
    
    // Connection rate limiting
    config->connection.max_connections = 100;
    config->connection.max_connections_per_ip = 5;
    config->connection.connection_timeout = std::chrono::seconds(60);
    config->connection.idle_timeout = std::chrono::seconds(300);
    
    // Transfer rate limiting
    config->transfer.max_transfer_rate = 1024 * 1024; // 1 MB/s global
    config->transfer.max_transfer_rate_per_user = 512 * 1024; // 512 KB/s per user
    config->transfer.max_file_size = 100 * 1024 * 1024; // 100 MB max file
    
    // Passive mode configuration
    config->passive.min_port = 49152;
    config->passive.max_port = 65535;
    config->passive.use_external_ip = true;
    config->passive.external_ip = "203.0.113.1";
    
    // Security settings
    config->security.max_login_attempts = 3;
    config->security.login_timeout = std::chrono::seconds(30);
    config->security.session_timeout = std::chrono::seconds(1800);
    
    // Logging for security monitoring
    config->logging.log_level = "INFO";
    config->logging.log_commands = true;
    config->logging.log_transfers = true;
    config->logging.log_errors = true;
    config->logging.log_security = true;
}
```

## 📊 Monitoring and Logging Examples

### Advanced Logging Configuration

#### Structured Logging Setup
```cpp
#include "simple-sftpd/logger.hpp"

void setupAdvancedLogging() {
    // Create main logger
    auto main_logger = std::make_shared<simple_sftpd::Logger>(
        "/var/log/simple-sftpd/simple-sftpd.log",
        simple_sftpd::LogLevel::INFO,
        true,  // log to console
        true   // log to file
    );
    
    // Configure log format
    main_logger->setLogFormat(simple_sftpd::LogFormat::EXTENDED);
    main_logger->setMaxLogSize(50 * 1024 * 1024); // 50 MB
    main_logger->setMaxLogFiles(10);
    
    // Create transfer logger
    auto transfer_logger = std::make_shared<simple_sftpd::Logger>(
        "/var/log/simple-sftpd/transfers.log",
        simple_sftpd::LogLevel::INFO,
        false, // don't log to console
        true   // log to file
    );
    
    transfer_logger->setLogFormat(simple_sftpd::LogFormat::JSON);
    transfer_logger->setMaxLogSize(100 * 1024 * 1024); // 100 MB
    
    // Create security logger
    auto security_logger = std::make_shared<simple_sftpd::Logger>(
        "/var/log/simple-sftpd/security.log",
        simple_sftpd::LogLevel::WARN,
        false, // don't log to console
        true   // log to file
    );
    
    security_logger->setLogFormat(simple_sftpd::LogFormat::EXTENDED);
    
    // Enable performance monitoring
    main_logger->setPerformanceMonitoring(true);
    main_logger->setMetricsCollection(true);
}
```

#### Custom Log Format
```cpp
#include "simple-sftpd/logger.hpp"

void setupCustomLogging() {
    auto logger = std::make_shared<simple_sftpd::Logger>();
    
    // Set custom log format
    std::string custom_format = "[{timestamp}] [{level}] [{thread}] [{file}:{line}] {message}";
    logger->setCustomFormat(custom_format);
    logger->setLogFormat(simple_sftpd::LogFormat::CUSTOM);
    
    // Set log level
    logger->setLogLevel(simple_sftpd::LogLevel::DEBUG);
    
    // Enable asynchronous logging
    logger->setAsyncLogging(true);
    logger->setAsyncBufferSize(1000);
    
    // Enable log rotation
    logger->setLogRotation(true);
    logger->setLogRotationTime("daily");
    logger->setLogRotationSize(10 * 1024 * 1024); // 10 MB
    
    // Set log file permissions
    logger->setLogFilePermissions(0644);
    
    // Enable compression for old logs
    logger->setLogCompression(true);
}
```

### Performance Monitoring

#### Performance Metrics Collection
```cpp
#include "simple-sftpd/logger.hpp"
#include "simple-sftpd/ftp_server.hpp"

void setupPerformanceMonitoring() {
    auto logger = std::make_shared<simple_sftpd::Logger>();
    
    // Enable performance monitoring
    logger->setPerformanceMonitoring(true);
    logger->setMetricsCollection(true);
    
    // Set metrics collection interval
    logger->setMetricsCollectionInterval(std::chrono::seconds(60));
    
    // Enable specific metrics
    logger->setMetricCollection("cpu_usage", true);
    logger->setMetricCollection("memory_usage", true);
    logger->setMetricCollection("disk_io", true);
    logger->setMetricCollection("network_io", true);
    logger->setMetricCollection("connection_count", true);
    logger->setMetricCollection("transfer_rate", true);
    
    // Set performance thresholds
    logger->setPerformanceThreshold("cpu_usage", 80.0); // 80% CPU usage
    logger->setPerformanceThreshold("memory_usage", 85.0); // 85% memory usage
    logger->setPerformanceThreshold("connection_count", 90); // 90 connections
    
    // Set alert callbacks
    logger->setPerformanceAlertCallback([](const std::string& metric, double value, double threshold) {
        std::cout << "Performance alert: " << metric << " = " << value 
                  << " (threshold: " << threshold << ")" << std::endl;
    });
}
```

## 🔧 Configuration Management Examples

### Dynamic Configuration Reloading

#### Hot Configuration Reload
```cpp
#include "simple-sftpd/ftp_server.hpp"
#include "simple-sftpd/ftp_server_config.hpp"
#include <filesystem>

void setupHotReload() {
    auto config = std::make_shared<simple_sftpd::FTPServerConfig>();
    auto server = std::make_shared<simple_sftpd::FTPServer>(config);
    
    // Start configuration file watcher
    std::thread config_watcher([&]() {
        std::string config_file = "/etc/simple-sftpd/simple-sftpd.conf";
        std::filesystem::file_time_type last_modified = 
            std::filesystem::last_write_time(config_file);
        
        while (server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            try {
                auto current_modified = std::filesystem::last_write_time(config_file);
                if (current_modified > last_modified) {
                    std::cout << "Configuration file changed, reloading..." << std::endl;
                    
                    if (server->reloadConfig()) {
                        std::cout << "Configuration reloaded successfully" << std::endl;
                        last_modified = current_modified;
                    } else {
                        std::cout << "Failed to reload configuration" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error checking configuration file: " << e.what() << std::endl;
            }
        }
    });
    
    // Start server
    if (server->start()) {
        std::cout << "Server started with hot reload enabled" << std::endl;
        
        // Wait for shutdown
        while (server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Clean up
    if (config_watcher.joinable()) {
        config_watcher.join();
    }
}
```

#### Configuration Validation
```cpp
#include "simple-sftpd/ftp_server_config.hpp"

bool validateConfiguration(const std::string& config_file) {
    auto config = std::make_shared<simple_sftpd::FTPServerConfig>();
    
    // Load configuration
    if (!config->loadFromFile(config_file)) {
        std::cerr << "Failed to load configuration file: " << config_file << std::endl;
        return false;
    }
    
    // Validate configuration
    if (!config->validate()) {
        auto errors = config->getErrors();
        std::cerr << "Configuration validation failed:" << std::endl;
        
        for (const auto& error : errors) {
            std::cerr << "  - " << error << std::endl;
        }
        
        return false;
    }
    
    // Additional custom validation
    if (config->connection.bind_port < 1024 && !config->security.drop_privileges) {
        std::cerr << "Warning: Binding to privileged port without dropping privileges" << std::endl;
    }
    
    if (config->ssl.enabled && config->ssl.certificate_file.empty()) {
        std::cerr << "Error: SSL enabled but no certificate file specified" << std::endl;
        return false;
    }
    
    if (config->ssl.enabled && config->ssl.private_key_file.empty()) {
        std::cerr << "Error: SSL enabled but no private key file specified" << std::endl;
        return false;
    }
    
    std::cout << "Configuration validation passed" << std::endl;
    return true;
}
```

## 🧪 Testing Examples

### Unit Test Examples

#### User Class Testing
```cpp
#include <gtest/gtest.h>
#include "simple-sftpd/ftp_user.hpp"

class FTPUserTest : public ::testing::Test {
protected:
    void SetUp() override {
        user = std::make_shared<simple_sftpd::FTPUser>("testuser");
    }
    
    std::shared_ptr<simple_sftpd::FTPUser> user;
};

TEST_F(FTPUserTest, Constructor) {
    EXPECT_EQ(user->getUsername(), "testuser");
    EXPECT_EQ(user->getStatus(), simple_sftpd::UserStatus::ACTIVE);
    EXPECT_TRUE(user->isEnabled());
}

TEST_F(FTPUserTest, PasswordVerification) {
    user->setPassword("testpass123");
    EXPECT_TRUE(user->verifyPassword("testpass123"));
    EXPECT_FALSE(user->verifyPassword("wrongpass"));
}

TEST_F(FTPUserTest, PermissionManagement) {
    user->grantPermission(simple_sftpd::UserPermission::READ);
    user->grantPermission(simple_sftpd::UserPermission::WRITE);
    
    EXPECT_TRUE(user->hasPermission(simple_sftpd::UserPermission::READ));
    EXPECT_TRUE(user->hasPermission(simple_sftpd::UserPermission::WRITE));
    EXPECT_FALSE(user->hasPermission(simple_sftpd::UserPermission::DELETE));
    
    user->revokePermission(simple_sftpd::UserPermission::WRITE);
    EXPECT_FALSE(user->hasPermission(simple_sftpd::UserPermission::WRITE));
}

TEST_F(FTPUserTest, HomeDirectory) {
    std::string home_dir = "/var/ftp/testuser";
    user->setHomeDirectory(home_dir);
    EXPECT_EQ(user->getHomeDirectory(), home_dir);
}
```

#### Virtual Host Testing
```cpp
#include <gtest/gtest.h>
#include "simple-sftpd/ftp_virtual_host.hpp"

class FTPVirtualHostTest : public ::testing::Test {
protected:
    void SetUp() override {
        vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("test.example.com");
    }
    
    std::shared_ptr<simple_sftpd::FTPVirtualHost> vhost;
};

TEST_F(FTPVirtualHostTest, Constructor) {
    EXPECT_EQ(vhost->getHostname(), "test.example.com");
    EXPECT_TRUE(vhost->getDocumentRoot().empty());
}

TEST_F(FTPVirtualHostTest, DocumentRoot) {
    std::string doc_root = "/var/ftp/test";
    vhost->setDocumentRoot(doc_root);
    EXPECT_EQ(vhost->getDocumentRoot(), doc_root);
}

TEST_F(FTPVirtualHostTest, SSLConfiguration) {
    simple_sftpd::SSLConfig ssl_config;
    ssl_config.enabled = true;
    ssl_config.certificate_file = "/path/to/cert.crt";
    
    vhost->setSSLConfig(ssl_config);
    
    const auto& config = vhost->getSSLConfig();
    EXPECT_TRUE(config.enabled);
    EXPECT_EQ(config.certificate_file, "/path/to/cert.crt");
}

TEST_F(FTPVirtualHostTest, UserAccess) {
    auto user = std::make_shared<simple_sftpd::FTPUser>("testuser");
    
    // Initially no users allowed
    EXPECT_FALSE(vhost->isUserAllowed("testuser"));
    
    // Add user to allowed list
    vhost->addAllowedUser("testuser");
    EXPECT_TRUE(vhost->isUserAllowed("testuser"));
    
    // Remove user from allowed list
    vhost->removeAllowedUser("testuser");
    EXPECT_FALSE(vhost->isUserAllowed("testuser"));
}
```

### Integration Test Examples

#### Server Integration Test
```cpp
#include <gtest/gtest.h>
#include "simple-sftpd/ftp_server.hpp"
#include "simple-sftpd/ftp_server_config.hpp"

class FTPServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_shared<simple_sftpd::FTPServerConfig>();
        config->connection.bind_port = 0; // Use random port
        config->connection.max_connections = 10;
        
        server = std::make_shared<simple_sftpd::FTPServer>(config);
    }
    
    void TearDown() override {
        if (server->isRunning()) {
            server->stop();
        }
    }
    
    std::shared_ptr<simple_sftpd::FTPServerConfig> config;
    std::shared_ptr<simple_sftpd::FTPServer> server;
};

TEST_F(FTPServerIntegrationTest, ServerStartStop) {
    EXPECT_FALSE(server->isRunning());
    
    EXPECT_TRUE(server->start());
    EXPECT_TRUE(server->isRunning());
    
    server->stop();
    EXPECT_FALSE(server->isRunning());
}

TEST_F(FTPServerIntegrationTest, UserManagement) {
    auto user = std::make_shared<simple_sftpd::FTPUser>("testuser");
    user->setPassword("testpass");
    user->setHomeDirectory("/var/ftp/testuser");
    
    server->addUser(user);
    
    auto retrieved_user = server->getUser("testuser");
    EXPECT_NE(retrieved_user, nullptr);
    EXPECT_EQ(retrieved_user->getUsername(), "testuser");
    
    server->removeUser("testuser");
    
    retrieved_user = server->getUser("testuser");
    EXPECT_EQ(retrieved_user, nullptr);
}

TEST_F(FTPServerIntegrationTest, VirtualHostManagement) {
    auto vhost = std::make_shared<simple_sftpd::FTPVirtualHost>("test.example.com");
    vhost->setDocumentRoot("/var/ftp/test");
    
    server->addVirtualHost(vhost);
    
    auto retrieved_vhost = server->getVirtualHost("test.example.com");
    EXPECT_NE(retrieved_vhost, nullptr);
    EXPECT_EQ(retrieved_vhost->getHostname(), "test.example.com");
    
    server->removeVirtualHost("test.example.com");
    
    retrieved_vhost = server->getVirtualHost("test.example.com");
    EXPECT_EQ(retrieved_vhost, nullptr);
}
```

## 🚀 Production Deployment Examples

### Systemd Service Configuration

#### Systemd Service File
```ini
# /etc/systemd/system/simple-sftpd.service
[Unit]
Description=Simple-Secure FTP Daemon
Documentation=man:simple-sftpd(8)
After=network.target

[Service]
Type=notify
User=ftp
Group=ftp
ExecStart=/usr/bin/simple-sftpd --config /etc/simple-sftpd/simple-sftpd.conf
ExecReload=/bin/kill -HUP $MAINPID
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal
SyslogIdentifier=simple-sftpd

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/ftp /var/log/simple-sftpd
CapabilityBoundingSet=CAP_NET_BIND_SERVICE

# Resource limits
LimitNOFILE=65536
LimitNPROC=1024

[Install]
WantedBy=multi-user.target
```

#### Systemd Service Management
```bash
#!/bin/bash
# /usr/local/bin/simple-sftpd-ctl

SERVICE_NAME="simple-sftpd"
CONFIG_FILE="/etc/simple-sftpd/simple-sftpd.conf"

case "$1" in
    start)
        echo "Starting Simple-Secure FTP Daemon..."
        systemctl start $SERVICE_NAME
        ;;
    stop)
        echo "Stopping Simple-Secure FTP Daemon..."
        systemctl stop $SERVICE_NAME
        ;;
    restart)
        echo "Restarting Simple-Secure FTP Daemon..."
        systemctl restart $SERVICE_NAME
        ;;
    reload)
        echo "Reloading Simple-Secure FTP Daemon configuration..."
        systemctl reload $SERVICE_NAME
        ;;
    status)
        systemctl status $SERVICE_NAME
        ;;
    enable)
        echo "Enabling Simple-Secure FTP Daemon to start on boot..."
        systemctl enable $SERVICE_NAME
        ;;
    disable)
        echo "Disabling Simple-Secure FTP Daemon from starting on boot..."
        systemctl disable $SERVICE_NAME
        ;;
    config-test)
        echo "Testing configuration file..."
        /usr/bin/simple-sftpd --config $CONFIG_FILE --test-config
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|reload|status|enable|disable|config-test}"
        exit 1
        ;;
esac

exit 0
```

### Docker Deployment

#### Dockerfile
```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libjsoncpp-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Clone and build Simple-Secure FTP Daemon
WORKDIR /opt
RUN git clone https://github.com/blburns/simple-sftpd.git
WORKDIR /opt/simple-sftpd
RUN mkdir build && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install

# Create FTP user and directories
RUN useradd -r -s /bin/false ftp \
    && mkdir -p /var/ftp /var/log/simple-sftpd \
    && chown ftp:ftp /var/ftp /var/log/simple-sftpd

# Copy configuration
COPY simple-sftpd.conf /etc/simple-sftpd/simple-sftpd.conf
RUN chown ftp:ftp /etc/simple-sftpd/simple-sftpd.conf

# Expose ports
EXPOSE 21 49152-65535

# Switch to FTP user
USER ftp

# Start the server
CMD ["/usr/local/bin/simple-sftpd", "--config", "/etc/simple-sftpd/simple-sftpd.conf"]
```

#### Docker Compose
```yaml
version: '3.8'

services:
  simple-sftpd:
    build: .
    container_name: simple-sftpd
    ports:
      - "21:21"
      - "49152-65535:49152-65535"
    volumes:
      - ./config:/etc/simple-sftpd
      - ./ftp-data:/var/ftp
      - ./logs:/var/log/simple-sftpd
    environment:
      - TZ=UTC
    restart: unless-stopped
    networks:
      - ftp-network

  ftp-admin:
    image: nginx:alpine
    container_name: ftp-admin
    ports:
      - "8080:80"
    volumes:
      - ./admin-html:/usr/share/nginx/html
      - ./admin-config:/etc/nginx/conf.d
    depends_on:
      - simple-sftpd
    restart: unless-stopped
    networks:
      - ftp-network

networks:
  ftp-network:
    driver: bridge

volumes:
  ftp-data:
  ftp-logs:
```

## 📚 Additional Resources

- **API Reference**: [API Documentation](../api/README.md)
- **Configuration Guide**: [Configuration Documentation](../configuration/README.md)
- **User Guide**: [User Documentation](../user-guide/README.md)
- **Development Guide**: [Development Documentation](../development/README.md)

For more examples and use cases, check the source code in the `examples/` directory or visit the project repository.
