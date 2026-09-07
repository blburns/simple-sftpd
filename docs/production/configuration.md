# Production Version Configuration Guide

**Version:** 0.4.0  
**License:** Apache 2.0

---

## Overview

Configuration for the Production Version of simple-sftpd. The server accepts **INI** (`.conf`), **JSON** (`.json`), and **YAML** (`.yml`/`.yaml`); format is detected from the file extension.

See [Configuration Reference](../shared/configuration/README.md) for the full option list.

## Example Locations

| Template | Path |
|----------|------|
| Minimal | `config/simple/` |
| Advanced (SSL, tuning) | `config/advanced/` |
| Production-hardened | `config/production/` |

## Key Production Options (v0.4.0)

```ini
[security]
user_file = /etc/simple-sftpd/users.json   # persistent local users
virtual_hosts_file = /etc/simple-sftpd/virtual_hosts.json  # virtual host definitions (CLI + runtime)
chroot_enabled = true
drop_privileges = true
run_as_user = ftp

[transfer]
enable_compression = false   # MODE Z on RETR/STOR when true

[virtual_hosts]
enable_virtual_hosts = true

[virtual_hosts.example]
hostname = ftp.example.com
document_root = /var/ftp/example
enabled = true
```

- **Virtual hosts:** Defined under `[virtual_hosts.<name>]`. Clients use the FTP **HOST** command after connect.
- **Users:** Stored in JSON when `security.user_file` is set; auto-loaded at startup.
- **Reload:** `simple-sftpd reload` or SIGHUP reloads most settings without restart.

Validate before deploy:

```bash
simple-sftpd test --config /etc/simple-sftpd/simple-sftpd.conf
```

---

**Last Updated:** August 2026
