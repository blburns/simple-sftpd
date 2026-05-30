# Code Reorganization Plan - Modular Structure

**Status:** ✅ **COMPLETE** (May 2026) — source reorganized under `include/simple-sftpd/` and `src/simple-sftpd/` by domain (core, config, user, virtual_host, security, utils).

## Current Structure
```
include/simple-sftpd/
  - ftp_server.hpp
  - ftp_connection.hpp
  - ftp_connection_manager.hpp
  - ftp_user.hpp
  - ftp_user_manager.hpp
  - ftp_virtual_host.hpp
  - ftp_virtual_host_manager.hpp
  - ftp_server_config.hpp
  - ftp_rate_limiter.hpp
  - ip_access_control.hpp
  - ssl_context.hpp
  - pam_auth.hpp
  - logger.hpp
  - compression.hpp
  - file_cache.hpp
  - performance_monitor.hpp
  - vulnerability_scanner.hpp

src/
  core/
    - ftp_server.cpp
    - ftp_connection.cpp
    - ftp_connection_manager.cpp
    - ftp_rate_limiter.cpp
    - ftp_user_manager.cpp
    - ftp_user.cpp
    - ftp_virtual_host_manager.cpp
    - ftp_virtual_host.cpp
  utils/
    - compression.cpp
    - file_cache.cpp
    - ftp_server_config.cpp
    - ip_access_control.cpp
    - logger.cpp
    - pam_auth.cpp
    - performance_monitor.cpp
    - ssl_context.cpp
    - vulnerability_scanner.cpp
```

## Proposed Modular Structure

### C++ Layered Architecture

```
include/simple-sftpd/
  core/              # Core FTP protocol layer
    - server.hpp      # Main FTP server orchestrator
    - connection.hpp  # Individual connection handler
    - connection_manager.hpp  # Connection management
  
  user/              # User management layer
    - user.hpp        # User entity
    - user_manager.hpp  # User management
  
  virtual_host/      # Virtual hosting layer
    - virtual_host.hpp  # Virtual host entity
    - virtual_host_manager.hpp  # Virtual host management
  
  security/          # Security layer
    - rate_limiter.hpp  # Rate limiting
    - ip_access_control.hpp  # IP access control
    - ssl_context.hpp  # SSL/TLS context
    - pam_auth.hpp  # PAM authentication
  
  config/            # Configuration layer
    - server_config.hpp  # Server configuration
  
  utils/             # Utility layer
    - logger.hpp      # Logging utilities
    - compression.hpp  # Compression utilities
    - file_cache.hpp  # File caching
    - performance_monitor.hpp  # Performance monitoring
    - vulnerability_scanner.hpp  # Security scanning

src/simple-sftpd/
  [same structure as include/]
```

## Benefits

1. **Clear Separation of Concerns**: Each layer has a specific responsibility
2. **Better Organization**: Related code is grouped together
3. **Easier Navigation**: Developers know where to find code
4. **Scalability**: Easy to add new features in appropriate layers
5. **Testability**: Each layer can be tested independently
6. **Maintainability**: Related code is grouped together

