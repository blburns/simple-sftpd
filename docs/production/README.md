# Production Version Documentation

**Version:** 0.1.0  
**License:** Apache 2.0  
**Status:** Production Ready

---

## Overview

The Production Version of Simple Secure FTP Daemon (simple-sftpd) is an FTP server suitable for small to medium deployments, with FTPS, security hardening, and multi-format configuration.

## What's Included

- Complete FTP protocol implementation (RFC 959)
- FTPS support (SSL/TLS), including per-host SSL certificates (v0.3.0)
- Passive and active mode
- Security: authentication, path validation, chroot, privilege dropping, IP access control, rate limiting, bandwidth throttling
- PAM authentication support
- **Virtual hosting (v0.3.0):** HOST command, per-host root and user manager, per-host SSL, quotas, session limits, custom error messages
- **User management (v0.3.0):** persistent user storage (JSON), groups, guest accounts with expiry, per-user storage quota
- Multi-format configuration (JSON, YAML, INI)
- Configuration reload (SIGHUP)
- Cross-platform support (Linux, macOS, Windows)

## Documentation

| Document | Description |
|----------|-------------|
| [Deployment](deployment.md) | Install, configure, and deploy in production |
| [Configuration](configuration.md) | Configuration reference and examples |
| [Operations](operations.md) | Day-to-day operations and service management |
| [Performance](performance.md) | Performance tuning and monitoring |
| [Security](security.md) | Security recommendations and hardening |
| [Troubleshooting](troubleshooting.md) | Common issues and solutions |

## Quick Start

1. Build and install (or use a package).
2. Copy and edit a config from `config/production/` or `config/simple/`.
3. Create directories (e.g. `/var/ftp`, `/var/log/simple-sftpd`, `/etc/simple-sftpd`).
4. Run `simple-sftpd test --config /path/to/config` to validate.
5. Start the service (e.g. `simple-sftpd start` or systemd/launchd).

See [Deployment](deployment.md) and [Operations](operations.md) for details.

---

**Last Updated:** March 2025  
**Version:** 0.1.0
