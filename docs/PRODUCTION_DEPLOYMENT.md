# Production Deployment Guide

Best practices for deploying simple-sftpd in production.

## Before you deploy

- **Backup:** Backup config, `users.json`, and any custom SSL/certs before changes.
- **Test in staging:** Run the same config and client workflows in a non-production environment first.
- **Plan maintenance:** Use a dedicated user/group, log rotation, and a clear upgrade/rollback plan.

---

## Security

### TLS/FTPS

- Prefer **FTPS** over plain FTP. Set `[ssl] enabled = true` and use a valid certificate (e.g. from a CA or internal PKI).
- Set `require_ssl = true` under `[security]` so unencrypted logins are rejected.
- Use strong ciphers and TLS 1.2+ (e.g. `min_tls_version = 1.2` or `1.3`). Avoid legacy ciphers.

### Run as non-root

- Set `drop_privileges = true` and `run_as_user` / `run_as_group` to a dedicated system user (e.g. `ftp` or `simple-sftpd`).
- Create that user with a non-login shell and a dedicated home (e.g. `/var/ftp`). Ensure all writable paths (home dirs, logs, temp) are owned by that user.

### Chroot and paths

- Enable chroot: `chroot_enabled = true`, `chroot_directory` set to the FTP root (e.g. `/var/ftp`).
- Give each user a home under the chroot. The server enforces path validation and blocks traversal; keep home dirs and permissions tight.

### Access control

- Use **IP allow/deny** if supported (e.g. `allowed_ips` / `denied_ips` or equivalent) to restrict which clients can connect.
- Limit `max_connections_per_ip` and use **rate_limit** to reduce abuse.
- Disable anonymous access in production unless required: `allow_anonymous = false`.

### Passwords and users

- Store `users.json` (and config) with restrictive permissions (e.g. root:ftp 0640). Don’t put passwords in config files; use the user store or PAM.
- Use strong passwords; consider PAM or future integration with an identity provider for centralised auth.

### Auditing and logging

- Use a **log format** that includes timestamps and user/session (e.g. JSON or EXTENDED) for audit.
- Send logs to a central logging system (e.g. syslog, SIEM) and retain according to policy.
- Enable security-related options (e.g. login and command logging) as needed for compliance.

---

## Performance and limits

- **Connections:** Set `max_connections` and `max_connections_per_ip` to match expected load and capacity.
- **Timeouts:** Tune `connection_timeout`, `data_timeout`, and `idle_timeout` so slow clients don’t hold resources forever.
- **Transfer:** Set `max_transfer_rate` and per-user limits to avoid one user saturating the link. Use `max_file_size` if appropriate.
- **Passive ports:** Allocate a dedicated passive port range and open it in the firewall; avoid overlapping with other services.

---

## Filesystem and directories

- **Home dirs:** One directory per user under the chroot. Set correct ownership (run user) and permissions (e.g. 0750 for home, 0644/0755 for files/dirs as needed).
- **Temp directory:** If used, put it on a writable volume and ensure only the run user can write. Clean or rotate it periodically.
- **Log directory:** Dedicated log dir, owned by run user; use logrotate or equivalent so logs don’t fill the disk.

---

## Service management

- **systemd (Linux):** Use the provided unit file or install script. Prefer `Type=simple` unless the binary supports `Type=notify`. Use `Restart=always` and a short `RestartSec`. Consider `LimitNOFILE` if you expect many connections.
- **launchd (macOS):** Install the plist under `/Library/LaunchDaemons/`, ensure paths match your install, create log/working dirs, then load/start the job.
- **Windows:** Use the NSSM-based install script; run as Administrator. Confirm executable, config, and log paths; set recovery options in NSSM if desired.

After any config change, reload or restart the service and run a quick smoke test (login, list, upload, download).

---

## Monitoring and health

- **Health check:** Use the FTP port (e.g. `nc -z host 21`) or a simple login script from a monitor. In containers, use the Docker HEALTHCHECK or equivalent.
- **Metrics:** If the server exposes stats (e.g. connections, transfers), integrate with your monitoring (Prometheus, Grafana, etc.).
- **Alerts:** Alert on service down, connection failures, disk space (logs and FTP storage), and any security-related log patterns.

---

## Firewall and network

- Allow **TCP 21** (control) and the **passive port range** (e.g. 49152–65535) from intended clients only when possible.
- If the server is behind NAT, set `passive.external_ip` (and optionally the passive port range) so PASV replies point to the public IP and open ports.
- Consider putting the server in a DMZ or restricted VLAN and limit which hosts can connect.

---

## Upgrades and maintenance

- Read release notes and any **migration guide** before upgrading.
- Test upgrades and config changes in staging. Backup config and user data.
- Prefer clean restarts during maintenance windows; use graceful stop (e.g. `systemctl stop`) so active transfers can finish if the server supports it.
- After upgrade, run `--test-config`, start the service, and re-run your usual client tests.

---

*See also: [Troubleshooting](TROUBLESHOOTING.md), [Configuration](configuration/README.md), [ROADMAP_CHECKLIST.md](../project/ROADMAP_CHECKLIST.md).*
