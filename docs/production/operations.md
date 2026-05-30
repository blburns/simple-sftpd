# Production Version Operations Guide

**Version:** 0.3.0  
**License:** Apache 2.0

---

## Overview

Day-to-day operations for the Production Version of simple-sftpd.

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

# Add user (local JSON store)
simple-sftpd user add <username> <password> [home_directory]

# Remove user
simple-sftpd user remove <username>
```

Users are persisted to JSON when `security.user_file` is set (default: `/etc/simple-sftpd/users.json` on Linux). The server auto-loads on startup and auto-saves on add/remove. Use PAM on Linux to authenticate against system accounts instead of or alongside the local store.

## Virtual Hosting

Manage virtual hosts via CLI (persisted to JSON, default `/etc/simple-sftpd/virtual_hosts.json`):

```bash
# List configured hosts
simple-sftpd virtual list

# Add a host
simple-sftpd virtual add --hostname ftp.example.com --root /var/ftp/example \
  --certificate /etc/ssl/certs/example.crt --private-key /etc/ssl/private/example.key

# Enable, disable, modify, or remove
simple-sftpd virtual disable --hostname ftp.example.com
simple-sftpd virtual modify --hostname ftp.example.com --root /var/ftp/newroot
simple-sftpd virtual remove --hostname ftp.example.com
```

Set `security.virtual_hosts_file` in the server config to override the storage path. Clients select a host with the FTP **HOST** command after connecting. Restart or reload the server after CLI changes so running instances pick up updates.

## Logs

- Default log path: `/var/log/simple-sftpd/simple-sftpd.log` (configurable).
- Use `log_level` (e.g. INFO or WARN) and `log_format` (STANDARD, JSON, or EXTENDED).
- Rotate logs via logrotate or your platform’s rotation (see `deployment/logrotate.d`).

## Configuration Reload

- Send SIGHUP or use `simple-sftpd reload` to reload configuration without dropping existing connections.
- Some options require a full restart (bind address/port, SSL certificate paths, chroot root).

## Health Checks

- Verify process and listen port: `simple-sftpd status`.
- Validate config before start/reload: `simple-sftpd test`.
- Optional external check: connect, login, LIST, QUIT.

---

**Last Updated:** May 2026  
**Version:** 0.3.0
