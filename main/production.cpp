#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <signal.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include "simple-sftpd/core/server.hpp"
#include "simple-sftpd/config/server_config.hpp"
#include "simple-sftpd/utils/logger.hpp"

#ifndef SIMPLE_SFTPD_VERSION
#define SIMPLE_SFTPD_VERSION "0.5.0"
#endif
#include "simple-sftpd/user/user_manager.hpp"
#include "simple-sftpd/user/user.hpp"
#include "simple-sftpd/virtual_host/virtual_host_manager.hpp"
#include "simple-sftpd/virtual_host/virtual_host.hpp"

using namespace simple_sftpd;

// Global variables for signal handling
std::shared_ptr<FTPServer> g_server;
std::shared_ptr<Logger> g_logger;
std::atomic<bool> g_shutdown_requested(false);

// Forward declarations
bool startServer(const std::string& config_file, bool daemon_mode);

namespace {

std::string defaultVirtualHostsFilePath() {
#ifdef _WIN32
    return "C:\\Program Files\\simple-sftpd\\virtual_hosts.json";
#else
    std::string path = "/etc/simple-sftpd/virtual_hosts.json";
    if (!std::filesystem::exists("/etc/simple-sftpd") && access("/etc/simple-sftpd", W_OK) != 0) {
        const char* home = getenv("HOME");
        path = std::string(home ? home : ".") + "/.simple-sftpd/virtual_hosts.json";
    }
    return path;
#endif
}

std::string resolveVirtualHostsFile(const std::string& config_file) {
    if (!config_file.empty() && std::filesystem::exists(config_file)) {
        auto config = std::make_shared<FTPServerConfig>();
        if (config->loadFromFile(config_file) && !config->security.virtual_hosts_file.empty()) {
            return config->security.virtual_hosts_file;
        }
    }
    return defaultVirtualHostsFilePath();
}

struct VirtualHostCliOptions {
    std::string hostname;
    std::string root;
    std::string cert;
    std::string key;
    std::string ca;
    std::string user_file;
    bool has_enabled = false;
    bool enabled = true;
    bool has_max_sessions = false;
    int max_sessions = 0;
    bool has_storage_quota = false;
    uint64_t storage_quota = 0;
    bool has_bandwidth_quota = false;
    uint64_t bandwidth_quota = 0;
};

VirtualHostCliOptions parseVirtualHostOptions(const std::vector<std::string>& args, size_t start) {
    VirtualHostCliOptions opts;
    for (size_t i = start; i < args.size(); ++i) {
        if (i + 1 >= args.size()) {
            continue;
        }
        const std::string& flag = args[i];
        const std::string& value = args[++i];
        if (flag == "--hostname" || flag == "-n") {
            opts.hostname = value;
        } else if (flag == "--root" || flag == "-r") {
            opts.root = value;
        } else if (flag == "--certificate" || flag == "--cert") {
            opts.cert = value;
        } else if (flag == "--private-key" || flag == "--key") {
            opts.key = value;
        } else if (flag == "--ca-certificate" || flag == "--ca") {
            opts.ca = value;
        } else if (flag == "--user-file") {
            opts.user_file = value;
        } else if (flag == "--max-sessions") {
            opts.has_max_sessions = true;
            opts.max_sessions = std::stoi(value);
        } else if (flag == "--storage-quota") {
            opts.has_storage_quota = true;
            opts.storage_quota = std::stoull(value);
        } else if (flag == "--bandwidth-quota") {
            opts.has_bandwidth_quota = true;
            opts.bandwidth_quota = std::stoull(value);
        } else if (flag == "--enabled") {
            opts.has_enabled = true;
            opts.enabled = (value == "true" || value == "1" || value == "yes");
        }
    }
    return opts;
}

void printVirtualHostDetails(const std::shared_ptr<FTPVirtualHost>& host) {
    std::cout << "  " << host->getHostname()
              << (host->isEnabled() ? " [enabled]" : " [disabled]")
              << std::endl;
    std::cout << "    root: " << host->getRootDirectory() << std::endl;
    if (!host->getSslCertFile().empty()) {
        std::cout << "    ssl_cert: " << host->getSslCertFile() << std::endl;
    }
    if (!host->getSslKeyFile().empty()) {
        std::cout << "    ssl_key: " << host->getSslKeyFile() << std::endl;
    }
    if (host->getMaxSessions() > 0) {
        std::cout << "    max_sessions: " << host->getMaxSessions() << std::endl;
    }
    if (host->getStorageQuotaBytes() > 0) {
        std::cout << "    storage_quota_bytes: " << host->getStorageQuotaBytes() << std::endl;
    }
    if (host->getUserManager() && !host->getUserManager()->getUserFile().empty()) {
        std::cout << "    user_file: " << host->getUserManager()->getUserFile() << std::endl;
    }
}

} // namespace

// PID file path
std::string getPidFile() {
    #ifdef _WIN32
    return "C:\\Program Files\\simple-sftpd\\run\\simple-sftpd.pid";
    #else
    return "/var/run/simple-sftpd.pid";
    #endif
}

/**
 * @brief Write PID to file
 */
void writePidFile() {
    std::string pid_file = getPidFile();
    std::ofstream file(pid_file);
    if (file.is_open()) {
        file << getpid() << std::endl;
        file.close();
    }
}

/**
 * @brief Read PID from file
 */
pid_t readPidFile() {
    std::string pid_file = getPidFile();
    std::ifstream file(pid_file);
    if (file.is_open()) {
        pid_t pid;
        file >> pid;
        file.close();
        return pid;
    }
    return -1;
}

/**
 * @brief Remove PID file
 */
void removePidFile() {
    std::string pid_file = getPidFile();
    if (std::filesystem::exists(pid_file)) {
        std::filesystem::remove(pid_file);
    }
}

/**
 * @brief Check if process is running
 */
bool isProcessRunning(pid_t pid) {
    if (pid <= 0) return false;
    #ifndef _WIN32
    return (kill(pid, 0) == 0);
    #else
    // Windows implementation would use OpenProcess
    return false;
    #endif
}

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number
 */
void signalHandler(int signal) {
    if (g_shutdown_requested.exchange(true)) {
        // Already shutting down, force exit
        std::exit(1);
    }

    if (g_logger) {
        g_logger->info("Received signal " + std::to_string(signal) + ", initiating graceful shutdown");
    }

    if (g_server) {
        g_server->stop();
    }
    removePidFile();
}

/**
 * @brief Print usage information
 */
void printUsage() {
    std::cout << "\nUsage: simple-sftpd [OPTIONS] [COMMAND] [ARGS...]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --help, -h           Show this help message" << std::endl;
    std::cout << "  --version, -v        Show version information" << std::endl;
    std::cout << "  --config, -c FILE    Use specified configuration file" << std::endl;
    std::cout << "  --verbose, -V        Enable verbose logging" << std::endl;
    std::cout << "  --daemon, -d         Run as daemon" << std::endl;
    std::cout << "  --foreground, -f     Run in foreground" << std::endl;
    std::cout << "  --test-config        Test configuration file" << std::endl;
    std::cout << "  --validate           Validate configuration" << std::endl;

    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start                Start the FTP server" << std::endl;
    std::cout << "  stop                 Stop the FTP server" << std::endl;
    std::cout << "  restart              Restart the FTP server" << std::endl;
    std::cout << "  status               Show server status" << std::endl;
    std::cout << "  reload               Reload configuration" << std::endl;
    std::cout << "  test                 Test server configuration" << std::endl;
    std::cout << "  user                 Manage users" << std::endl;
    std::cout << "  virtual              Manage virtual hosts" << std::endl;
    std::cout << "  ssl                  Manage SSL certificates" << std::endl;

    std::cout << "\nUser Subcommands:" << std::endl;
    std::cout << "  add                  Add new user" << std::endl;
    std::cout << "  remove               Remove user" << std::endl;
    std::cout << "  modify               Modify user" << std::endl;
    std::cout << "  list                 List users" << std::endl;
    std::cout << "  password             Change user password" << std::endl;

    std::cout << "\nVirtual Host Subcommands:" << std::endl;
    std::cout << "  add                  Add new virtual host" << std::endl;
    std::cout << "  remove               Remove virtual host" << std::endl;
    std::cout << "  modify               Modify virtual host" << std::endl;
    std::cout << "  list                 List virtual hosts" << std::endl;
    std::cout << "  enable               Enable virtual host" << std::endl;
    std::cout << "  disable              Disable virtual host" << std::endl;

    std::cout << "\nSSL Subcommands:" << std::endl;
    std::cout << "  generate             Generate self-signed certificate" << std::endl;
    std::cout << "  install              Install certificate" << std::endl;
    std::cout << "  renew                Renew certificate" << std::endl;
    std::cout << "  status               Show SSL status" << std::endl;

    std::cout << "\nExamples:" << std::endl;
    std::cout << "  simple-sftpd start --config /etc/simple-sftpd/config.json" << std::endl;
    std::cout << "  simple-sftpd user add --username john --password secret --home /home/john" << std::endl;
    std::cout << "  simple-sftpd virtual add --hostname ftp.example.com --root /var/ftp/example" << std::endl;
    std::cout << "  simple-sftpd ssl generate --hostname ftp.example.com" << std::endl;
    std::cout << "  simple-sftpd --daemon start" << std::endl;
}

/**
 * @brief Print version information
 */
void printVersion() {
    std::cout << "simple-sftpd v" << SIMPLE_SFTPD_VERSION << std::endl;
    std::cout << "Simple FTP Daemon for Linux, macOS, and Windows" << std::endl;
    std::cout << "Copyright (c) 2024 SimpleDaemons" << std::endl;
}

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param config_file Output configuration file path
 * @param command Output command
 * @param args Output command arguments
 * @param daemon_mode Output daemon mode flag
 * @param foreground_mode Output foreground mode flag
 * @param verbose Output verbose flag
 * @return true if parsed successfully, false otherwise
 */
bool parseArguments(int argc, char* argv[],
                   std::string& config_file,
                   std::string& command,
                   std::vector<std::string>& args,
                   bool& daemon_mode,
                   bool& foreground_mode,
                   bool& verbose) {
    config_file.clear();
    command.clear();
    args.clear();
    daemon_mode = false;
    foreground_mode = false;
    verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return false;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return false;
            }
        } else if (arg == "--verbose" || arg == "-V") {
            verbose = true;
        } else if (arg == "--daemon" || arg == "-d") {
            daemon_mode = true;
        } else if (arg == "--foreground" || arg == "-f") {
            foreground_mode = true;
        } else if (arg == "--test-config") {
            command = "test-config";
        } else if (arg == "--validate") {
            command = "validate";
        } else if (arg[0] != '-') {
            // This is a command or argument
            if (command.empty()) {
                command = arg;
            } else {
                args.push_back(arg);
            }
        } else {
            // This is an option for the command, add it to args
            if (!command.empty()) {
                args.push_back(arg);
            }
        }
    }

    return true;
}

/**
 * @brief Setup signal handlers
 */
void setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    #ifndef _WIN32
    signal(SIGHUP, signalHandler);
    #endif
}

/**
 * @brief Daemonize the process
 * @return true if successful, false otherwise
 */
bool daemonize() {
    #ifdef _WIN32
    // Windows daemonization would be different
    return false;
    #else
    // Fork the process
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid > 0) {
        // Parent process, exit
        exit(0);
    }

    // Child process continues
    // Create new session
    if (setsid() < 0) {
        return false;
    }

    // Change working directory
    if (chdir("/") < 0) {
        return false;
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect to /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);

    return true;
    #endif
}

/**
 * @brief Test configuration file
 * @param config_file Configuration file path
 * @return true if valid, false otherwise
 */
bool testConfiguration(const std::string& config_file) {
    auto config = std::make_shared<FTPServerConfig>();

    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
        return false;
    }

    if (!config->validate()) {
        std::cerr << "Error: Configuration validation failed:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return false;
    }

    std::cout << "Configuration file is valid: " << config_file << std::endl;
    return true;
}

/**
 * @brief Validate configuration
 * @param config_file Configuration file path
 * @return true if valid, false otherwise
 */
bool validateConfiguration(const std::string& config_file) {
    auto config = std::make_shared<FTPServerConfig>();

    if (!config->loadFromFile(config_file)) {
        std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
        return false;
    }

    std::cout << "Configuration validation results:" << std::endl;
    std::cout << "  File: " << config_file << std::endl;
    std::cout << "  Loaded: " << (config->getErrors().empty() ? "Yes" : "No") << std::endl;

    if (!config->getErrors().empty()) {
        std::cout << "  Errors:" << std::endl;
        for (const auto& error : config->getErrors()) {
            std::cout << "    " << error << std::endl;
        }
    }

    if (!config->getWarnings().empty()) {
        std::cout << "  Warnings:" << std::endl;
        for (const auto& warning : config->getWarnings()) {
            std::cout << "    " << warning << std::endl;
        }
    }

    return config->getErrors().empty();
}

/**
 * @brief Stop the FTP server
 * @return true if stopped successfully, false otherwise
 */
bool stopServer() {
    pid_t pid = readPidFile();
    if (pid <= 0) {
        std::cout << "Server is not running (no PID file found)" << std::endl;
        return false;
    }

    if (!isProcessRunning(pid)) {
        std::cout << "Server process not found (PID: " << pid << ")" << std::endl;
        removePidFile();
        return false;
    }

    std::cout << "Stopping server (PID: " << pid << ")..." << std::endl;
    #ifndef _WIN32
    if (kill(pid, SIGTERM) == 0) {
        // Wait for process to terminate
        int count = 0;
        while (isProcessRunning(pid) && count < 30) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
        }
        if (isProcessRunning(pid)) {
            std::cout << "Server did not stop gracefully, sending SIGKILL..." << std::endl;
            kill(pid, SIGKILL);
        }
        removePidFile();
        std::cout << "Server stopped successfully" << std::endl;
        return true;
    }
    #endif
    return false;
}

/**
 * @brief Restart the FTP server
 * @param config_file Configuration file path
 * @param daemon_mode Run as daemon
 * @return true if restarted successfully, false otherwise
 */
bool restartServer(const std::string& config_file, bool daemon_mode) {
    std::cout << "Restarting server..." << std::endl;
    if (stopServer()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return startServer(config_file, daemon_mode);
    }
    return false;
}

/**
 * @brief Show server status
 * @param config_file Configuration file path
 */
void showStatus(const std::string& config_file) {
    pid_t pid = readPidFile();
    
    std::cout << "Server Status:" << std::endl;
    std::cout << "  PID File: " << getPidFile() << std::endl;
    
    if (pid > 0 && isProcessRunning(pid)) {
        std::cout << "  Status: Running" << std::endl;
        std::cout << "  PID: " << pid << std::endl;
        
        // Try to load config and show info
        auto config = std::make_shared<FTPServerConfig>();
        if (config->loadFromFile(config_file)) {
            std::cout << "  Listen Address: " << config->connection.bind_address << std::endl;
            std::cout << "  Listen Port: " << config->connection.bind_port << std::endl;
            std::cout << "  Max Connections: " << config->connection.max_connections << std::endl;
        }
    } else {
        std::cout << "  Status: Stopped" << std::endl;
        if (pid > 0) {
            removePidFile();
        }
    }
}

/**
 * @brief Reload configuration
 * @return true if reloaded successfully, false otherwise
 */
bool reloadConfiguration() {
    pid_t pid = readPidFile();
    if (pid <= 0 || !isProcessRunning(pid)) {
        std::cout << "Server is not running" << std::endl;
        return false;
    }

    std::cout << "Reloading configuration (PID: " << pid << ")..." << std::endl;
    #ifndef _WIN32
    if (kill(pid, SIGHUP) == 0) {
        std::cout << "Configuration reload signal sent" << std::endl;
        std::cout << "Note: Full configuration reload requires server restart in v0.1.0" << std::endl;
        return true;
    }
    #endif
    return false;
}

/**
 * @brief Handle user management commands
 * @param args Command arguments
 * @param config_file Configuration file path
 * @return true if successful, false otherwise
 */
bool handleUserCommand(const std::vector<std::string>& args, const std::string& config_file) {
    if (args.empty()) {
        std::cerr << "Error: user command requires a subcommand (add, remove, modify, list, password)" << std::endl;
        return false;
    }

    std::string subcommand = args[0];
    
    // Determine user file path
    std::string user_file;
    #ifdef _WIN32
    user_file = "C:\\Program Files\\simple-sftpd\\users.json";
    #else
    user_file = "/etc/simple-sftpd/users.json";
    // Fallback to local directory if /etc is not writable
    if (!std::filesystem::exists("/etc/simple-sftpd") && 
        access("/etc/simple-sftpd", W_OK) != 0) {
        user_file = std::string(getenv("HOME") ? getenv("HOME") : ".") + "/.simple-sftpd/users.json";
    }
    #endif
    
    // Initialize logger and user manager with user file
    auto logger = std::make_shared<Logger>("", LogLevel::INFO, true, false, LogFormat::STANDARD);
    auto user_manager = std::make_shared<FTPUserManager>(logger, user_file);

    if (subcommand == "add") {
        std::string username, password, home_dir;
        for (size_t i = 1; i < args.size(); ++i) {
            if (i + 1 < args.size()) {
                if (args[i] == "--username" || args[i] == "-u") {
                    username = args[++i];
                } else if (args[i] == "--password" || args[i] == "-p") {
                    password = args[++i];
                } else if (args[i] == "--home" || args[i] == "-h") {
                    home_dir = args[++i];
                }
            }
        }
        
        if (username.empty() || password.empty() || home_dir.empty()) {
            std::cerr << "Error: user add requires --username, --password, and --home" << std::endl;
            return false;
        }
        
        auto user = std::make_shared<FTPUser>(username, password, home_dir);
        if (user_manager->addUser(user)) {
            std::cout << "User '" << username << "' added successfully" << std::endl;
            std::cout << "User saved to: " << user_manager->getUserFile() << std::endl;
            return true;
        }
        return false;
        
    } else if (subcommand == "remove") {
        std::string username;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--username" || args[i] == "-u") {
                if (i + 1 < args.size()) {
                    username = args[++i];
                }
            }
        }
        
        if (username.empty()) {
            std::cerr << "Error: user remove requires --username" << std::endl;
            return false;
        }
        
        if (user_manager->removeUser(username)) {
            std::cout << "User '" << username << "' removed successfully" << std::endl;
            std::cout << "Changes saved to: " << user_manager->getUserFile() << std::endl;
            return true;
        } else {
            std::cerr << "Error: User '" << username << "' not found" << std::endl;
            return false;
        }
        
    } else if (subcommand == "list") {
        auto usernames = user_manager->listUsers();
        if (usernames.empty()) {
            std::cout << "No users found" << std::endl;
        } else {
            std::cout << "Users:" << std::endl;
            for (const auto& name : usernames) {
                auto user = user_manager->getUser(name);
                if (user) {
                    std::cout << "  " << name << " (home: " << user->getHomeDirectory() << ")" << std::endl;
                }
            }
        }
        return true;
        
    } else if (subcommand == "modify" || subcommand == "password") {
        std::cout << "User modification not yet fully implemented in v0.1.0" << std::endl;
        std::cout << "Use 'user remove' and 'user add' to change user properties" << std::endl;
        return false;
        
    } else {
        std::cerr << "Error: Unknown user subcommand: " << subcommand << std::endl;
        return false;
    }
}

/**
 * @brief Handle virtual host management commands
 * @param args Command arguments
 * @param config_file Configuration file path (optional, for virtual_hosts_file)
 * @return true if successful, false otherwise
 */
bool handleVirtualCommand(const std::vector<std::string>& args, const std::string& config_file) {
    if (args.empty()) {
        std::cerr << "Error: virtual command requires a subcommand (add, remove, modify, list, enable, disable)" << std::endl;
        return false;
    }

    const std::string subcommand = args[0];
    const std::string vhost_file = resolveVirtualHostsFile(config_file);
    auto logger = std::make_shared<Logger>("", LogLevel::INFO, true, false, LogFormat::STANDARD);
    auto manager = std::make_shared<FTPVirtualHostManager>(logger, vhost_file);

    if (subcommand == "list") {
        auto hosts = manager->getAllVirtualHosts();
        if (hosts.empty()) {
            std::cout << "No virtual hosts configured" << std::endl;
            std::cout << "Storage file: " << manager->getVirtualHostsFile() << std::endl;
        } else {
            std::cout << "Virtual hosts (" << hosts.size() << "):" << std::endl;
            for (const auto& host : hosts) {
                printVirtualHostDetails(host);
            }
            std::cout << "Storage file: " << manager->getVirtualHostsFile() << std::endl;
        }
        return true;
    }

    VirtualHostCliOptions opts = parseVirtualHostOptions(args, 1);

    if (subcommand == "add") {
        if (opts.hostname.empty() || opts.root.empty()) {
            std::cerr << "Error: virtual add requires --hostname and --root" << std::endl;
            return false;
        }
        auto host = std::make_shared<FTPVirtualHost>(opts.hostname, opts.root);
        if (opts.has_enabled) {
            host->setEnabled(opts.enabled);
        }
        if (!opts.cert.empty()) {
            host->setSslCertFile(opts.cert);
        }
        if (!opts.key.empty()) {
            host->setSslKeyFile(opts.key);
        }
        if (!opts.ca.empty()) {
            host->setSslCaFile(opts.ca);
        }
        if (opts.has_max_sessions) {
            host->setMaxSessions(opts.max_sessions);
        }
        if (opts.has_storage_quota) {
            host->setStorageQuotaBytes(opts.storage_quota);
        }
        if (opts.has_bandwidth_quota) {
            host->setBandwidthQuotaBytes(opts.bandwidth_quota);
        }
        if (!opts.user_file.empty()) {
            host->setUserManager(std::make_shared<FTPUserManager>(logger, opts.user_file));
        }
        if (!manager->addVirtualHost(host)) {
            std::cerr << "Error: Failed to add virtual host '" << opts.hostname
                      << "' (already exists or could not save to "
                      << manager->getVirtualHostsFile() << ")" << std::endl;
            return false;
        }
        std::cout << "Virtual host '" << opts.hostname << "' added" << std::endl;
        std::cout << "Saved to: " << manager->getVirtualHostsFile() << std::endl;
        std::cout << "Restart or reload the server for running instances to pick up changes." << std::endl;
        return true;
    }

    if (subcommand == "remove") {
        if (opts.hostname.empty()) {
            std::cerr << "Error: virtual remove requires --hostname" << std::endl;
            return false;
        }
        if (!manager->removeVirtualHost(opts.hostname)) {
            std::cerr << "Error: Virtual host '" << opts.hostname << "' not found" << std::endl;
            return false;
        }
        std::cout << "Virtual host '" << opts.hostname << "' removed" << std::endl;
        std::cout << "Saved to: " << manager->getVirtualHostsFile() << std::endl;
        return true;
    }

    if (subcommand == "enable" || subcommand == "disable") {
        if (opts.hostname.empty()) {
            std::cerr << "Error: virtual " << subcommand << " requires --hostname" << std::endl;
            return false;
        }
        auto host = manager->getVirtualHost(opts.hostname);
        if (!host) {
            std::cerr << "Error: Virtual host '" << opts.hostname << "' not found" << std::endl;
            return false;
        }
        host->setEnabled(subcommand == "enable");
        if (!manager->saveVirtualHosts()) {
            std::cerr << "Error: Failed to save virtual hosts file" << std::endl;
            return false;
        }
        std::cout << "Virtual host '" << opts.hostname << "' "
                  << (subcommand == "enable" ? "enabled" : "disabled") << std::endl;
        return true;
    }

    if (subcommand == "modify") {
        if (opts.hostname.empty()) {
            std::cerr << "Error: virtual modify requires --hostname" << std::endl;
            return false;
        }
        auto host = manager->getVirtualHost(opts.hostname);
        if (!host) {
            std::cerr << "Error: Virtual host '" << opts.hostname << "' not found" << std::endl;
            return false;
        }
        if (!opts.root.empty()) {
            host->setRootDirectory(opts.root);
        }
        if (opts.has_enabled) {
            host->setEnabled(opts.enabled);
        }
        if (!opts.cert.empty()) {
            host->setSslCertFile(opts.cert);
        }
        if (!opts.key.empty()) {
            host->setSslKeyFile(opts.key);
        }
        if (!opts.ca.empty()) {
            host->setSslCaFile(opts.ca);
        }
        if (opts.has_max_sessions) {
            host->setMaxSessions(opts.max_sessions);
        }
        if (opts.has_storage_quota) {
            host->setStorageQuotaBytes(opts.storage_quota);
        }
        if (opts.has_bandwidth_quota) {
            host->setBandwidthQuotaBytes(opts.bandwidth_quota);
        }
        if (!opts.user_file.empty()) {
            host->setUserManager(std::make_shared<FTPUserManager>(logger, opts.user_file));
        }
        if (!manager->saveVirtualHosts()) {
            std::cerr << "Error: Failed to save virtual hosts file" << std::endl;
            return false;
        }
        std::cout << "Virtual host '" << opts.hostname << "' updated" << std::endl;
        std::cout << "Saved to: " << manager->getVirtualHostsFile() << std::endl;
        return true;
    }

    std::cerr << "Error: Unknown virtual subcommand: " << subcommand << std::endl;
    return false;
}

/**
 * @brief Handle SSL management commands
 * @param args Command arguments
 * @param config_file Configuration file path
 * @return true if successful, false otherwise
 */
bool handleSslCommand(const std::vector<std::string>& args, const std::string& config_file) {
    if (args.empty()) {
        std::cerr << "Error: ssl command requires a subcommand" << std::endl;
        return false;
    }

    std::string subcommand = args[0];
    
    if (subcommand == "status") {
        std::cout << "SSL/TLS Status:" << std::endl;
        
        // Check build-time SSL support
        #ifdef SIMPLE_SFTPD_SSL_ENABLED
        std::cout << "  Build support: ✅ Enabled (OpenSSL available)" << std::endl;
        #else
        std::cout << "  Build support: ❌ Disabled (OpenSSL not available)" << std::endl;
        std::cout << "  Note: Rebuild with OpenSSL to enable SSL/TLS support" << std::endl;
        return true;
        #endif
        
        // Load configuration to check runtime status
        auto config = std::make_shared<FTPServerConfig>();
        bool config_loaded = false;
        
        if (!config_file.empty() && std::filesystem::exists(config_file)) {
            config_loaded = config->loadFromFile(config_file);
        }
        
        if (config_loaded) {
            bool ssl_configured = !config->security.ssl_cert_file.empty() && 
                                  !config->security.ssl_key_file.empty();
            
            std::cout << "  Configuration: " << (ssl_configured ? "✅ Configured" : "⚠️  Not configured") << std::endl;
            
            if (ssl_configured) {
                std::cout << "  Certificate: " << config->security.ssl_cert_file << std::endl;
                std::cout << "  Private Key: " << config->security.ssl_key_file << std::endl;
                
                // Check if files exist
                bool cert_exists = std::filesystem::exists(config->security.ssl_cert_file);
                bool key_exists = std::filesystem::exists(config->security.ssl_key_file);
                
                std::cout << "  Certificate file: " << (cert_exists ? "✅ Found" : "❌ Not found") << std::endl;
                std::cout << "  Private key file: " << (key_exists ? "✅ Found" : "❌ Not found") << std::endl;
                
                if (cert_exists && key_exists) {
                    std::cout << "  Status: ✅ Ready for SSL/TLS connections" << std::endl;
                } else {
                    std::cout << "  Status: ⚠️  Configuration incomplete (files missing)" << std::endl;
                }
                
                if (config->security.require_ssl) {
                    std::cout << "  Mode: Required (all connections must use SSL)" << std::endl;
                } else {
                    std::cout << "  Mode: Optional (AUTH TLS available)" << std::endl;
                }
            } else {
                std::cout << "  Status: ⚠️  SSL/TLS not configured" << std::endl;
                std::cout << "  To enable SSL/TLS, configure ssl_cert_file and ssl_key_file" << std::endl;
            }
        } else {
            std::cout << "  Configuration: ⚠️  Could not load config file" << std::endl;
            if (!config_file.empty()) {
                std::cout << "  Config file: " << config_file << std::endl;
            }
            std::cout << "  Note: SSL/TLS is implemented and ready to use" << std::endl;
            std::cout << "  Configure ssl_cert_file and ssl_key_file in your config to enable" << std::endl;
        }
        
        std::cout << std::endl << "  Certificate generation:" << std::endl;
        std::cout << "    Use: tools/setup-ssl.sh --hostname <hostname>" << std::endl;
        
        return true;
    } else if (subcommand == "generate") {
        std::cout << "SSL certificate generation:" << std::endl;
        std::cout << "  Please use: tools/setup-ssl.sh --hostname <hostname>" << std::endl;
        std::cout << "  This will generate a self-signed certificate for testing" << std::endl;
        std::cout << "  For production, use certificates from a trusted CA" << std::endl;
        return true;
    } else {
        std::cout << "Unknown SSL subcommand: " << subcommand << std::endl;
        std::cout << "Available subcommands: status, generate" << std::endl;
        return false;
    }
}

/**
        return false;
    }
}

 * @brief Start the FTP server
 * @param config_file Configuration file path
 * @param daemon_mode Run as daemon
 * @return true if started successfully, false otherwise
 */
bool startServer(const std::string& config_file, bool daemon_mode) {
    try {
        // Load configuration
        auto config = std::make_shared<FTPServerConfig>();
        if (!config->loadFromFile(config_file)) {
            std::cerr << "Failed to load configuration from " << config_file << std::endl;
            return false;
        }

        if (!config->validate()) {
            std::cerr << "Error: Configuration validation failed:" << std::endl;
            for (const auto& error : config->getErrors()) {
                std::cerr << "  " << error << std::endl;
            }
            return false;
        }

        // Initialize logger
        LogFormat log_format = LogFormat::STANDARD;
        if (config->logging.log_format == "JSON") {
            log_format = LogFormat::JSON;
        } else if (config->logging.log_format == "EXTENDED") {
            log_format = LogFormat::EXTENDED;
        }
        
        g_logger = std::make_shared<Logger>(
            config->logging.log_file,
            config->logging.log_level == "TRACE" ? LogLevel::TRACE :
            config->logging.log_level == "DEBUG" ? LogLevel::DEBUG :
            config->logging.log_level == "INFO" ? LogLevel::INFO :
            config->logging.log_level == "WARN" ? LogLevel::WARN :
            config->logging.log_level == "ERROR" ? LogLevel::ERROR :
            config->logging.log_level == "FATAL" ? LogLevel::FATAL : LogLevel::INFO,
            config->logging.log_to_console,
            config->logging.log_to_file,
            log_format
        );

        g_logger->info(std::string("Starting Simple FTP Daemon v") + SIMPLE_SFTPD_VERSION);
        g_logger->info("Configuration file: " + config_file);

        // Create and start server
        g_server = std::make_shared<FTPServer>(config);

        if (!g_server->start()) {
            g_logger->error("Failed to start FTP server");
            return false;
        }

        // Write PID file
        writePidFile();

        g_logger->info("FTP server started successfully");
        g_logger->info("Listening on " + config->connection.bind_address + ":" + std::to_string(config->connection.bind_port));

        // Main server loop
        while (!g_shutdown_requested && g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        g_logger->info("FTP server shutdown complete");
        removePidFile();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error starting FTP server: " << e.what() << std::endl;
        removePidFile();
        return false;
    }
}

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string config_file;
    std::string command;
    std::vector<std::string> args;
    bool daemon_mode = false;
    bool foreground_mode = false;
    bool verbose = false;

    if (!parseArguments(argc, argv, config_file, command, args, daemon_mode, foreground_mode, verbose)) {
        return 0; // Help or version was printed
    }

    // Set default configuration file if none specified
    if (config_file.empty()) {
        #ifdef _WIN32
        config_file = "C:\\Program Files\\simple-sftpd\\config\\simple-sftpd.conf";
        #else
        config_file = "/etc/simple-sftpd/simple-sftpd.conf";
        #endif
    }

    // Handle special commands
    if (command == "test-config") {
        return testConfiguration(config_file) ? 0 : 1;
    }

    if (command == "validate") {
        return validateConfiguration(config_file) ? 0 : 1;
    }

    // Handle server management commands
    if (command == "stop") {
        return stopServer() ? 0 : 1;
    }

    if (command == "status") {
        showStatus(config_file);
        return 0;
    }

    if (command == "restart") {
        setupSignalHandlers();
        return restartServer(config_file, daemon_mode) ? 0 : 1;
    }

    if (command == "reload") {
        return reloadConfiguration() ? 0 : 1;
    }

    if (command == "test") {
        return testConfiguration(config_file) ? 0 : 1;
    }

    // Handle user management
    if (command == "user") {
        return handleUserCommand(args, config_file) ? 0 : 1;
    }

    // Handle virtual host management
    if (command == "virtual") {
        return handleVirtualCommand(args, config_file) ? 0 : 1;
    }

    // Handle SSL management
    if (command == "ssl") {
        return handleSslCommand(args, config_file) ? 0 : 1;
    }

    // Setup signal handlers for server commands
    if (command.empty() || command == "start") {
        setupSignalHandlers();
    }

    // Handle daemon mode
    if (daemon_mode && !foreground_mode && (command.empty() || command == "start")) {
        if (!daemonize()) {
            std::cerr << "Error: Failed to daemonize process" << std::endl;
            return 1;
        }
    }

    // Start server if no specific command or start command
    if (command.empty() || command == "start") {
        if (!startServer(config_file, daemon_mode)) {
            return 1;
        }
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << "Use 'simple-sftpd --help' for usage information" << std::endl;
        return 1;
    }

    return 0;
}
