# Security Checklist

Use this checklist for deployment and before security review or penetration testing.

## Configuration

- [ ] SSL/TLS enabled (`[ssl] enabled = true`) in production
- [ ] `require_ssl = true` so plain FTP logins are rejected
- [ ] Strong TLS: `min_tls_version = 1.2` or `1.3`; no weak ciphers
- [ ] Valid certificate (not self-signed in production unless intended)
- [ ] Chroot enabled (`chroot_enabled = true`) and `chroot_directory` set
- [ ] Privilege dropping enabled (`drop_privileges = true`) with dedicated user/group
- [ ] Anonymous access disabled (`allow_anonymous = false`) unless required
- [ ] Rate limiting enabled with sensible limits
- [ ] IP allow/deny (if supported) configured for known clients/networks
- [ ] Config and `users.json` file permissions restrictive (e.g. 0640); not world-readable

## Users and auth

- [ ] Strong passwords; no default or shared passwords
- [ ] User home dirs and permissions correct; no unnecessary write where not needed
- [ ] PAM or external auth (if used) configured and tested
- [ ] Session/timeouts and max login attempts set

## Network and OS

- [ ] Firewall allows only port 21 and passive range from required sources
- [ ] Server runs as non-root with minimal privileges
- [ ] Logs sent to a secure location; log dir not world-writable
- [ ] No unnecessary services or ports exposed on the same host

## Operational

- [ ] Run vulnerability scanner (e.g. `VulnerabilityScanner` or project scripts) and fix findings
- [ ] Log level and audit options suitable for incident response
- [ ] Backup and restore of config and user data tested
- [ ] Upgrade path and security advisories followed

---

*See [PRODUCTION_DEPLOYMENT.md](../docs/PRODUCTION_DEPLOYMENT.md) and [TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md).*
