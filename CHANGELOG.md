# Changelog

All notable changes to simple-sftpd will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2026-05-30

### Added
- **Virtual hosting (Production v0.3.0)**
  - HOST command with per-host root directory, user manager, and SSL certificates
  - Per-host session, storage, and bandwidth quotas; custom FTP error messages
- **Advanced user management**
  - User groups; guest accounts with expiration
  - Persistent JSON user storage with auto load/save (`security.user_file`)
  - SessionTracker for per-user and per-host concurrent session limits
- **Transfer performance**
  - sendfile and memory-mapped I/O paths for RETR (Linux/macOS)
  - Connection pool support in FTPConnectionManager

### Changed
- Product version documentation updated for v0.3.0 feature-complete state
- Build automation extended for multi-platform remote builds (Debian, RHEL, FreeBSD, macOS)

## [0.2.0] - 2026-03-16

### Added
- **Security & performance (Production v0.2.0)**
  - Connection pooling, file metadata cache, vulnerability scanner
  - IPv6 dual-stack listening; audit logging for auth and file operations
  - Bandwidth throttling on uploads and downloads
- **Transfer operations**
  - APPE, REST resume, RNFR/RNTO rename
  - Active mode (PORT) data connections

### Changed
- ROADMAP and project docs mark Production v0.2.0 security/performance items complete

## [0.1.0] - 2025-11-27

### Added
- **Multi-format Configuration Support**
  - JSON configuration file parsing (`.json` files)
  - YAML configuration file parsing (`.yml`, `.yaml` files)
  - Automatic format detection based on file extension
  - INI format support (`.conf` or no extension)
  
- **JSON Logging**
  - Structured JSON log output format
  - Configurable via `log_format = "JSON"` in configuration
  - Case-insensitive log format parsing
  - Proper JSON escaping for log messages

- **Core FTP Protocol Implementation**
  - Full RFC 959 compliance with all core commands
  - USER, PASS, QUIT, PWD, CWD, LIST, RETR, STOR, DELE, MKD, RMD, SIZE, TYPE, NOOP, SYST, FEAT
  - File transfers (upload/download) through data connections
  - Passive mode (PASV) support
  - Active mode (PORT) support

- **Security Features**
  - Path validation and directory traversal protection
  - Permission-based access control (read, write, list)
  - User home directory enforcement
  - Rate limiting (time-window based)
  - IP access control and whitelisting
  - PAM (Pluggable Authentication Modules) integration
  - Privilege dropping (setuid/setgid) for security hardening
  - Bandwidth throttling for uploads and downloads

- **SSL/TLS Support (FTPS)**
  - OpenSSL integration for secure data transfer
  - AUTH TLS command support
  - PBSZ and PROT commands
  - SSL certificate and key management
  - Client certificate authentication support

- **User Management**
  - User authentication (username/password)
  - CLI-based user management (add, remove, list)
  - In-memory user storage
  - PAM authentication support

- **Command-Line Interface**
  - Server management: start, stop, restart, status, reload, test
  - User management: user add, remove, list
  - SSL management: ssl status, generate
  - Virtual host management: virtual list (stub)
  - PID file management
  - Signal handling (SIGINT, SIGTERM, SIGHUP)

- **File Operations**
  - File upload (STOR)
  - File download (RETR)
  - File append (APPE)
  - File resume (REST)
  - File rename (RNFR/RNTO)
  - File deletion (DELE)
  - Directory creation (MKD)
  - Directory removal (RMD)
  - Directory listing (LIST)
  - File size query (SIZE)

- **Logging System**
  - Multiple log formats: STANDARD, JSON, EXTENDED
  - Configurable log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
  - Console and file output
  - Thread-safe logging
  - Timestamp with millisecond precision

- **Connection Management**
  - Multi-threaded connection handling
  - Connection tracking and cleanup
  - Maximum connection limits
  - Connection timeout handling
  - Thread-safe connection management

- **Build System**
  - CMake build system with multi-platform support
  - Traditional Makefile support
  - CPack package generation (DMG, PKG, DEB, RPM, TGZ, NSIS)
  - Cross-platform compilation (Linux, macOS, Windows)

- **Testing Infrastructure**
  - Google Test integration
  - 46 unit and integration tests
  - Automated test execution via CTest
  - Test coverage for core components

- **Documentation**
  - Comprehensive API documentation
  - User guide and configuration guide
  - Getting started guide
  - Development guide
  - Example configurations (simple, advanced, production)
  - Deployment guides (Docker, systemd, launchd)

- **Service Integration**
  - systemd service file
  - launchd plist file
  - Windows service files
  - Service installation scripts

### Changed
- Improved log format parsing to be case-insensitive
- Enhanced configuration system with format detection
- Updated build system to support JSON library linking

### Fixed
- Fixed compilation errors related to PAM authentication
- Fixed active mode data connection handling
- Fixed SSL CLI status messages
- Fixed download bandwidth throttling
- Fixed rate limiter integration in connection acceptance

### Technical Details
- C++17 standard
- Thread-safe implementation
- Cross-platform compatibility
- Modern CMake practices
- Comprehensive error handling

---

## [0.1.0-alpha] - 2024-12-XX

### Added
- Initial alpha release
- Basic FTP server functionality
- Core command implementation
- Basic authentication
- Configuration system (INI only)
- Logging system (STANDARD format only)

---

[0.3.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.3.0
[0.2.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.2.0
[0.1.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.1.0
[0.1.0-alpha]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.1.0-alpha

