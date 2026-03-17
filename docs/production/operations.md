# Production Version Operations Guide

**Version:** 0.1.0  
**License:** Apache 2.0

---

## Overview

This guide covers day-to-day operations for the Production Version of Simple Secure FTP Daemon (simple-sftpd).

## Common Commands

```bash
# Start server (foreground)
simple-sftpd start --config /etc/simple-sftpd/simple-sftpd.conf

# Stop server
simple-sftpd stop

# Restart server
simple-sftpd restart

# Check status
simple-sftpd status

# Test configuration
simple-sftpd test --config /etc/simple-sftpd/simple-sftpd.conf

# Reload configuration (SIGHUP)
simple-sftpd reload
```

## Service Management

### systemd (Linux)

```bash
sudo systemctl start simple-sftpd
sudo systemctl stop simple-sftpd
sudo systemctl restart simple-sftpd
sudo systemctl status simple-sftpd
sudo systemctl reload simple-sftpd
```

### launchd (macOS)

```bash
sudo launchctl load /Library/LaunchDaemons/com.simple-sftpd.simple-sftpd.plist
sudo launchctl unload /Library/LaunchDaemons/com.simple-sftpd.simple-sftpd.plist
```

## User Management

```bash
# List users
simple-sftpd user list

# Add user (when using local user store)
simple-sftpd user add <username> <password> [home_directory]

# Remove user
simple-sftpd user remove <username>
```

**Note:** In v0.1.0, users are stored in-memory only; they are lost on restart. Use PAM or plan for user persistence in a future release for production user management.

## Logs

- Default log path: `/var/log/simple-sftpd/simple-sftpd.log` (configurable).
- Use `log_level` (e.g. INFO or WARN) and `log_format` (STANDARD or JSON) in config.
- Rotate logs via logrotate or your platform’s rotation (see `deployment/logrotate.d`).

## Configuration Reload

- Send SIGHUP to the daemon or use `simple-sftpd reload` to reload configuration without dropping existing connections.
- Some options may require a full restart (e.g. bind address/port, SSL certificate path).

## Health Checks

- Verify process is running and listening: `simple-sftpd status`.
- Use `simple-sftpd test` to validate config before start/reload.
- Optionally use an external FTP health check (connect, login, LIST, QUIT) for monitoring.

---

**Last Updated:** March 2025  
**Version:** 0.1.0
