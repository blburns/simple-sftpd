# Simple Secure FTP Daemon - Troubleshooting Guide

**Version:** 0.1.0  
**Last Updated:** March 2025

---

## Quick Reference

### Common Commands

```bash
# Check server status
simple-sftpd status

# View logs
tail -f /var/log/simple-sftpd/simple-sftpd.log

# Test configuration
simple-sftpd test --config /etc/simple-sftpd/simple-sftpd.conf

# Restart service (systemd)
sudo systemctl restart simple-sftpd

# Reload configuration
simple-sftpd reload
```

---

## FTP Reply Codes (Summary)

| Code Range | Meaning |
|------------|---------|
| 1xx | Positive preliminary (operation started) |
| 2xx | Positive completion |
| 3xx | Positive intermediate (need more info) |
| 4xx | Transient negative (temporary failure, retry) |
| 5xx | Permanent negative (error) |

Common codes: `530` (login incorrect), `550` (file/permission error), `425`/`426` (data connection issues).

---

## Common Problems & Solutions

### Server won't start

**Symptoms:** Service fails to start; errors in logs or on console.

**Checks:**

```bash
# Is port 21 in use?
sudo lsof -i :21
sudo netstat -tlnp | grep :21

# Validate config
simple-sftpd test --config /etc/simple-sftpd/simple-sftpd.conf

# Check logs
sudo journalctl -u simple-sftpd -n 50
tail -100 /var/log/simple-sftpd/simple-sftpd.log
```

**Typical causes:**

- **Port 21 already in use:** Stop the other service or change `bind_port` in config.
- **Invalid config:** Fix syntax/values reported by `simple-sftpd test`.
- **Missing SSL files:** If SSL is enabled, ensure `certificate_file` and `private_key_file` exist and are readable by the daemon.
- **Permission issues:** Run as correct user; ensure log directory and chroot directory exist and have correct ownership.

---

### Login fails (530 or authentication errors)

**Checks:**

- User exists in local user manager or in PAM (if PAM is enabled).
- Password correct; no typos in username (case-sensitive).
- PAM: ensure PAM config and system users are set up correctly.
- If FTPS required: client must use AUTH TLS (or equivalent) before sending credentials.

**v0.1.0 note:** Users added via CLI are in-memory only and are lost on restart. Restart requires re-adding users or using PAM.

---

### Data connection / transfer failures (425, 426, timeouts)

**Passive mode:**

- Server opens a high port for data; client must connect to it.
- **Firewall:** Allow TCP range `passive.min_port`–`passive.max_port` (e.g. 49152–65535) on the server and any NAT.
- **NAT:** If client reaches server via public IP, set `passive.external_ip` to that public IP so PASV response is correct.

**Active mode:**

- Client must accept incoming connections from the server on the port it announces (PORT command).
- Client firewall or NAT often blocks this; passive mode is usually easier in production.

**Checks:**

```bash
# Server listening on passive range?
sudo ss -tlnp | grep -E '49152|49153'
```

---

### SSL/TLS (FTPS) errors

- **Certificate/key not found:** Check paths in config; ensure files exist and permissions are correct.
- **Certificate rejected by client:** Use a valid cert (e.g. from a CA or a client-trusted self-signed); check expiry.
- **Cipher mismatch:** Adjust `cipher_suite` to match client capabilities; avoid deprecated ciphers.

Run `simple-sftpd ssl status` to confirm SSL is enabled and config/cert paths are reported correctly.

---

### High CPU or poor performance

- Reduce log level (e.g. to WARN) and use file logging only.
- Enable and tune rate limiting and bandwidth throttling.
- Check `max_connections` and connection timeouts; limit per-IP if needed.
- Ensure adequate disk I/O and network capacity; see [Performance](performance.md).

---

### Users disappear after restart

In v0.1.0, local user store is in-memory only. Options:

- Re-add users after each restart via CLI, or
- Use PAM so system users are used for authentication (no persistence in simple-sftpd itself).

User persistence (file or database) is planned for a future release.

---

## Getting Help

- Run `simple-sftpd test` and fix any reported config errors.
- Capture relevant log lines (with timestamps) and FTP client error messages.
- Note server version: `simple-sftpd --version` (or equivalent).
- Check [configuration](configuration.md), [deployment](deployment.md), and [operations](operations.md) for correct setup.

---

**Last Updated:** March 2025  
**Version:** 0.1.0
