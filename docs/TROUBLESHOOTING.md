# Troubleshooting simple-sftpd

Common issues, log locations, and how to get help.

## Log locations

| Platform   | Default log path |
|-----------|-------------------|
| Linux     | `/var/log/simple-sftpd/simple-sftpd.log` or value of `log_file` in config |
| macOS     | `/usr/local/var/log/simple-sftpd/simple-sftpd.log` (or path in launchd plist) |
| Windows   | `C:\Program Files\simple-sftpd\logs\` or path set in service (NSSM) |
| systemd   | `journalctl -u simple-sftpd -f` |

Set `log_level = DEBUG` in config for more detail (avoid in production long-term).

---

## Server won’t start

**Symptom:** Process exits immediately or service fails to start.

- **Config error:** Run `simple-sftpd --test-config /path/to/config.conf`. Fix any reported errors (syntax, missing sections, invalid paths).
- **Port in use:** Another process is using port 21 (or your `bind_port`). On Linux: `ss -tlnp | grep :21` or `lsof -i :21`. Change `bind_port` or stop the other service.
- **Permission:** Binding to 21 usually requires root or `CAP_NET_BIND_SERVICE`. Start as root then drop privileges, or use a port ≥ 1024 for testing.
- **Missing SSL files:** If `[ssl] enabled = true`, ensure `certificate_file` and `private_key_file` exist and are readable by the run user.
- **Missing user file:** If `users.json` path is wrong or missing and no users are created, the server may still start with a default test user; check logs.

**Check:** Run in foreground without daemon: `simple-sftpd --config /path/to/config.conf` (no `--daemon`) and watch stderr/log.

---

## Can’t connect (connection refused / timeout)

- **Firewall:** Allow TCP port 21 (control) and the passive port range (e.g. 49152–65535). Examples:
  - Linux (firewalld): `firewall-cmd --permanent --add-port=21/tcp --add-port=49152-65535/tcp && firewall-cmd --reload`
  - Linux (ufw): `ufp allow 21/tcp && ufw allow 49152:65535/tcp && ufw reload`
- **Bind address:** If `bind_address` is not `0.0.0.0`, clients from other hosts may be refused. Use `0.0.0.0` for “all interfaces” (or the correct IP).
- **Passive mode:** Client uses passive (PASV). Server must be reachable on the passive port range from the client. If the server is behind NAT, set `passive.external_ip` (and optionally `passive.min_port`/`max_port`) so the client gets the right IP and ports.

---

## Login fails (530 Login incorrect)

- **User/password:** Ensure the user exists in `users.json` (or PAM) and the password is correct. For local users, use the CLI to add/change: `simple-sftpd user add <name> <password> <homedir>` (syntax may vary; see README).
- **PAM:** If PAM is enabled, ensure the PAM service is configured and the account is valid on the system.
- **SSL required:** If `require_ssl` is true, connect with FTPS (e.g. port 990 or AUTH TLS) instead of plain FTP.

---

## Data connection / list or transfer fails (425, 426)

- **Passive:** Client uses PASV; server opens a high port. Firewall must allow that range (see “Can’t connect” above). Logs often show “Can’t open data connection” or “425”.
- **Active (PORT):** Client connects back to the server. Server must be able to reach the client’s reported IP/port; often fails through NAT or strict client firewalls. Prefer passive in those cases.
- **Timeout:** Large or slow transfers may hit `data_timeout` or `idle_timeout`. Increase in config if needed.

---

## Permission denied (550) on files or dirs

- **Path validation:** Server restricts users to their home directory and blocks path traversal. Use paths relative to home; avoid `../`.
- **User permissions:** Each user has read/write/list flags. Ensure the user has the right permission (e.g. “read” for RETR, “write” for STOR, “list” for LIST).
- **Chroot:** With chroot, the user only sees the chroot tree. Paths in config (e.g. home dirs) must be valid inside the chroot.
- **OS permissions:** The run user (e.g. `run_as_user`) must have OS read/write on the underlying files and directories.

---

## Service install (systemd / launchd / Windows)

- **systemd:** If the unit uses `Type=simple`, the process must stay in foreground. Don’t use `--daemon` when started by systemd. If the script or unit uses `Type=notify`, the binary must support sd_notify; otherwise use `Type=simple`.
- **launchd:** Check paths in the plist (ProgramArguments, WorkingDirectory, StandardOutPath) match your install. Ensure the log/working dirs exist and are owned by the launchd user.
- **Windows (NSSM):** Confirm executable path, config path, and log directory in the batch file. Run install script as Administrator. Check NSSM event log or service “Properties” for errors.

---

## Getting help

1. Run with `--test-config` and fix any config errors.
2. Reproduce with `log_level = DEBUG` and capture the relevant log lines (and last few lines before failure).
3. Note: OS, install method (package/build), config (redact secrets), and exact client command or GUI used.
4. Open an issue (e.g. GitHub) or use project support channels, and attach the above.

---

*See also: [Configuration guide](configuration/README.md), [Production deployment](PRODUCTION_DEPLOYMENT.md).*
