# Simple Secure FTP Daemon (simple-sftpd) - Roadmap

This document outlines the development roadmap for simple-sftpd. For item-level tracking, see [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md).

## Project Vision

A modern, secure FTP server with:

- Enterprise-grade security (FTPS, PAM, chroot, access control)
- High performance (connection pooling, sendfile, memory-mapped I/O)
- Easy deployment (Docker, packages, Ansible build automation)
- Cross-platform support (Linux, macOS, FreeBSD, Windows)
- Flexible configuration (INI, JSON, YAML) and virtual hosting

## Current State

**Production line:** feature-complete through **v0.3.0** (released 2026-05-30)  
**Enterprise / Datacenter:** planned (BSL 1.1), 0% complete

| Version | Status | Summary |
|---------|--------|---------|
| **0.1.0** | ✅ Released (2025-11-27) | Core FTP, passive mode, config, logging, CLI, tests |
| **0.2.0** | ✅ Complete | FTPS, PAM, chroot, active mode, resume/append/rename, pooling, IPv6, audit logging |
| **0.3.0** | ✅ Complete | Virtual hosting (HOST), persistent users, groups, guest accounts, quotas, session limits |
| **0.4.0+** | ⏳ Planned | Compression on wire, CLI virtual-host management, Enterprise features |

### Production polish (post–v0.3.0)

- ⏳ Wire `Compression` into RETR/STOR (class implemented, not integrated)
- ⏳ Expand test coverage (~40% → 60%+ target)
- ⏳ Environment verification (systemd, launchd, Windows service, Docker, packages)
- ⏳ Wire `Compression` into RETR/STOR (class implemented, not integrated)

---

## Version 0.1.0 - Foundation Release

**Status:** ✅ **RELEASED**

- Core FTP protocol (RFC 959), passive mode, file transfers
- User authentication, permissions, path validation
- INI/JSON/YAML configuration, logging (STANDARD/JSON/EXTENDED)
- CLI (start, stop, restart, status, reload, test, user)
- CMake/Makefile build, Docker examples, Google Test suite

---

## Version 0.2.0 - Security & Performance

**Status:** ✅ **COMPLETE**

**Security:** FTPS (OpenSSL), PAM, chroot, privilege dropping, IP access control, rate limiting, bandwidth throttling, audit logging, vulnerability scanner

**Performance:** Connection pooling, file metadata cache, sendfile/mmap transfers, IPv6 dual-stack

---

## Version 0.3.0 - Virtual Hosting & Advanced User Management

**Status:** ✅ **COMPLETE**

**Virtual hosting:** HOST command, per-host root/users/SSL, quotas, session limits, custom error messages

**User management:** Persistent JSON storage (`security.user_file`), groups, guest accounts with expiry, per-user storage quotas, SessionTracker

---

## Version 0.4.0 - Production Polish (Planned)

**Target:** Q3 2026

- On-the-wire compression (gzip/bzip2) in transfer path
- `simple-sftpd virtual` CLI implementation
- Expanded integration tests (SSL, PAM, virtual hosts, sessions)
- Packaging and service verification on all target platforms

---

## Enterprise Version (Planned)

**License:** BSL 1.1 · **Target:** Large deployments, multi-server environments

- Web administration UI and REST API
- SNMP integration
- LDAP/Active Directory, RBAC, 2FA
- High availability and clustering
- Plugin architecture

See [docs/enterprise/README.md](docs/enterprise/README.md).

---

## Datacenter Version (Planned)

**License:** BSL 1.1 · **Target:** Cloud and multi-site operations

- Horizontal scaling and load balancing
- Multi-site synchronization
- Cloud storage backends (S3, Azure, GCP)
- Multi-tenant support and advanced analytics

See [docs/datacenter/README.md](docs/datacenter/README.md).

---

## Success Metrics (Long-Term Targets)

| Area | Target |
|------|--------|
| Concurrent connections | 10,000+ |
| Test coverage | 90%+ |
| Documentation | Complete API and ops coverage |
| Security | Zero known unpatched vulnerabilities |

---

**Legend:** ✅ Complete · 🔄 In progress · ⏳ Planned · ❌ Cancelled

*Last Updated: May 2026*
