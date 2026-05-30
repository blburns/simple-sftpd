# Production Version Security Guide

**Version:** 0.3.0  
**License:** Apache 2.0

---

## Overview

This guide covers security for the Production Version of Simple Secure FTP Daemon (simple-sftpd).

## Recommendations

### Use FTPS (SSL/TLS)

- Enable SSL/TLS in configuration and use valid certificates.
- Prefer TLS 1.2 or 1.3; disable SSLv3 and weak ciphers.
- Set `require_ssl` if all access must be encrypted.

### Authentication

- Prefer PAM when integrating with system users; otherwise use the built-in user manager.
- Disable `allow_anonymous` and `allow_guest` in production unless required.
- Use strong passwords and consider `max_login_attempts` and `login_timeout`.

### Access Control

- Use `chroot_enabled` and `chroot_directory` to restrict users to a subtree.
- Use `drop_privileges` and `run_as_user` / `run_as_group` to run as a dedicated user.
- Use IP allowlists/blacklists (IP access control) where applicable.

### Rate Limiting

- Enable rate limiting to reduce abuse: `rate_limit.enabled`, `max_connections_per_ip`, `max_requests_per_minute`.
- Use bandwidth throttling to avoid a single client monopolizing capacity.

### File and Path Security

- Path validation and directory traversal protection are built in; keep home directories and chroot settings correct.
- Restrict upload types via configuration if your build supports extension filters.

### Logging and Auditing

- Use JSON or EXTENDED log format and send logs to a secure, rotated file.
- Monitor logs for failed logins and unusual activity.

---

**Last Updated:** May 2026  
**Version:** 0.3.0
