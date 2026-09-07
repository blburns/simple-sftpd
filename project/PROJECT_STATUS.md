# Simple-Secure FTP Daemon - Project Status

## Project Overview

Simple Secure FTP Daemon is an FTP/FTPS server written in C++17 with:

- Multi-platform deployment (Linux, macOS, Windows)
- Core FTP plus MODE Z, PASV, PORT/EPRT
- User management, virtual hosting, and authentication
- Logging, CLI, and multi-format configuration

## Product Versions

### Production Version (Apache 2.0)

- **Status:** Complete through **v0.4.0** (v0.1.0 released; v0.2.0/v0.3.0 feature work complete)
- **Target:** Small to medium deployments, single-server installations
- **Features:** FTP/FTPS, security hardening, virtual hosting, persistent users, MODE Z, protocol tests
- **Documentation:** `docs/production/`

### Enterprise Version (BSL 1.1)

- **Status:** Planned — 0% complete
- **Features:** Web UI, REST API, SNMP, HA, clustering, LDAP/2FA
- **Documentation:** `docs/enterprise/`

### Datacenter Version (BSL 1.1)

- **Status:** Planned — 0% complete
- **Features:** Horizontal scaling, multi-site sync, cloud backends, multi-tenant
- **Documentation:** `docs/datacenter/`

## Completed Features (Production)

- FTP control + data connections (PASV, PORT, EPRT)
- RETR/STOR/REST/APPE/rename; MODE Z streaming zlib when enabled
- FTPS (OpenSSL), PAM (Linux), chroot, privilege drop, IP ACL, rate limits
- Virtual hosting (HOST), `virtual` CLI, persistent JSON users
- Groups, quotas, session limits, guest expiry
- Connection pooling, sendfile/mmap (skipped under MODE Z)
- CMake/CPack, Google Test + CTest
- macOS Phase 1: config test, launchd user-agent, native CPack `.pkg`

## Current Status

Production polish is **done**. Next product line is Enterprise (planned). Environment gaps: systemd/Docker/Windows not run on this Mac — see [VERIFICATION.md](VERIFICATION.md).

## Metrics

- Commands: core RFC 959 plus HOST, MODE, EPRT, AUTH/PBSZ/PROT
- Tests: ~59 (1 PAM skip on macOS); `ctest` green
- Platforms: Linux, macOS, Windows (Windows unverified here)
- Packages: DMG/PKG, DEB/RPM/TGZ, NSIS (macOS PKG built here)

## Next Steps

1. Tag **v0.4.0** when asked ([RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md))
2. Optional: Linux/Docker/Windows verification on those hosts
3. Enterprise kickoff when prioritized

## Project Health

**Status:** Production line complete through v0.4.0.

**Open (non-blocking):** coverage/load bars, Linux/Windows/Docker smoke, memory/perf review.

---

*Last Updated: August 2026*  
*Project Status: Production Version — complete through v0.4.0*
