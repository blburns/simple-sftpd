# Changelog

All notable changes to simple-sftpd will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.6.0] - 2026-09-07

### Security
- **Passwords are no longer stored or compared in plaintext.** The local user
  store now holds salted PBKDF2-HMAC-SHA256 hashes (210,000 iterations, 16-byte
  random salt) in a self-describing `$pbkdf2-sha256$<iterations>$<salt>$<digest>`
  field, so the cost factor can be raised later without invalidating entries.
- Password comparison is constant time, for both hashed and legacy values.
- Existing user files are migrated automatically: plaintext entries are hashed
  on load and the file is rewritten, so no manual conversion step is needed.
- `user add` and `user password` prompt for the password without echo when
  `--password` is omitted, keeping the credential out of the process list.
  A piped password is accepted when stdin is not a terminal.

### Added
- **Path-scoped permissions** — permission entries may now be scoped to a
  subtree (`write:/uploads`) in addition to bare operations (`read`, `all`).
  `FTPUser::hasPermission` previously ignored its `path` argument entirely.
  Permissions round-trip through the user file.
- `user password` and `user modify`, which previously printed
  "not yet fully implemented in v0.1.0" and returned failure. There was no way
  to rotate a password without deleting and recreating the account.
- `user add --permissions` and `user modify --permissions`.

### Changed
- OpenSSL's libcrypto is now always required; `ENABLE_SSL` gates only the
  FTPS/TLS feature set. Password hashing must not depend on a build option.
- `FTPUser::getPassword()` is now `getPasswordHash()`, reflecting that the
  stored value is a hash and is intended for serialization only.

## [0.5.0] - 2026-09-07

### Added
- **RFC command completeness** — the dispatcher previously answered `502` for a dozen standard commands:
  - `CDUP`/`XCUP` (RFC 959) parent-directory navigation
  - `EPSV` including `EPSV ALL` (RFC 2428), so IPv6 and NAT'd clients can use passive mode
  - `MLSD`/`MLST` machine-readable listings and `MDTM` modification times (RFC 3659)
  - `STAT` (server status, and per-path status over the control connection)
  - `OPTS` (`UTF8`, `MLST`), `ABOR`, `STOU`, `SITE` (`HELP`, `CHMOD`, `UMASK`), `ALLO`, `HELP`
- FEAT now advertises `EPSV`, `MDTM`, `MLST`, `TVFS`, and `UTF8`
- Protocol tests for every new command, plus an explicit path-traversal test

### Fixed
- **Path containment was unsound**: `validatePath()` re-ran `resolvePath()` on an
  already-resolved path, re-applying the home prefix to real absolute paths, and
  fell back to a substring compare when canonicalization failed. Containment now
  compares whole path components against the home and virtual-host roots, and
  uses `weakly_canonical` so not-yet-created upload targets are checked correctly.
- Integration tests no longer read or write the developer's real `~/.simple-sftpd`
  user and virtual-host stores; the fixture is now hermetic.

## [0.4.0] - 2026-08-30

### Added
- **On-the-wire compression (MODE Z)** — streaming zlib on RETR/STOR when `transfer.enable_compression` is set; advertised in FEAT
- **EPRT** active-mode command (RFC 2428, IPv4)
- Live protocol integration tests: PASV RETR/STOR, PORT, HOST, MODE Z, AUTH TLS smoke

### Fixed
- PORT was implemented but not dispatched (clients got `502`); active mode now works
- Accepted control sockets are forced blocking so login does not drop on macOS
- Connection-manager stop no longer waits on a 60s cleanup sleep
- REST/APPE rejected with a clear reply when MODE Z is active (not mixed with resume/append)

### Changed
- Production polish complete; Enterprise / Datacenter remain planned
- Config: `enable_compression` honored under `[transfer]` (and top-level INI)
- Packaging matches simple-ldapd: CPack FHS layout, `{name}-{version}-{platform}` names, macOS PKG rebuilt without CPack’s leaked `Contents/` payload, DEB/RPM maintainer scripts that do not start the daemon. Docker is not the packaging path.

## [0.3.0] - 2026-05-30

### Added
- **Virtual hosting (Production v0.3.0)**
  - HOST command with per-host root directory, user manager, and SSL certificates
  - Per-host session, storage, and bandwidth quotas; custom FTP error messages
  - `simple-sftpd virtual` CLI (add, list, modify, enable, disable, remove) with JSON persistence (`security.virtual_hosts_file`)
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
- README, ROADMAP, and production/development docs aligned with current application state

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

[0.4.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.4.0
[0.3.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.3.0
[0.2.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.2.0
[0.1.0]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.1.0
[0.1.0-alpha]: https://github.com/simpledaemons/simple-sftpd/releases/tag/v0.1.0-alpha

