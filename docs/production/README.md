# Production Version Documentation

**Version:** 0.4.0  
**License:** Apache 2.0  
**Status:** Feature-complete (Production line v0.1.0–v0.4.0)

---

## Overview

The Production Version of Simple Secure FTP Daemon (simple-sftpd) is an FTP/FTPS server for small to medium deployments, with security hardening, virtual hosting, and multi-format configuration.

## What's Included

- Complete FTP protocol (RFC 959), passive and active mode
- FTPS (SSL/TLS), including per-host SSL certificates
- Security: authentication, path validation, chroot, privilege dropping, IP access control, rate limiting, bandwidth throttling, PAM (Linux)
- **Virtual hosting:** HOST command, per-host root and user manager, per-host SSL, quotas, session limits, custom error messages
- **User management:** persistent JSON storage (`security.user_file`), groups, guest accounts with expiry, per-user storage quota
- Transfer optimization: sendfile and memory-mapped I/O (Linux/macOS), connection pooling
- On-the-wire **MODE Z** (streaming zlib) when `transfer.enable_compression` is set
- Multi-format configuration (INI, JSON, YAML) with SIGHUP reload
- Cross-platform: Linux, macOS, FreeBSD, Windows
- `simple-sftpd virtual` CLI (add/list/modify/enable/disable/remove)

## Out of Production scope

- Enterprise / Datacenter (web UI, REST, HA, LDAP)
- Full packaging/service verification on Linux and Windows (see [VERIFICATION.md](../../project/VERIFICATION.md))

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

1. Build and install (or use a package). See [Build Guide](../development/BUILD_GUIDE.md).
2. Copy and edit a config from `config/production/`, `config/simple/`, or `config/advanced/`.
3. Create directories (e.g. `/var/ftp`, `/var/log/simple-sftpd`, `/etc/simple-sftpd`).
4. Run `simple-sftpd test --config /path/to/config` to validate.
5. Start the service (`simple-sftpd start` or systemd/launchd).

See [Deployment](deployment.md) and [Operations](operations.md) for details.

---

**Last Updated:** August 2026  
**Version:** 0.4.0
