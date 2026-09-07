# Simple Secure FTP Daemon - Progress Report

**Date:** August 2026  
**Current Version:** Production v0.4.0  
**Overall Project Completion:** Production Version (Apache 2.0) — complete through v0.4.0 polish  
**Product Versions:** Production (Apache 2.0 — finished through v0.4.0), Enterprise (BSL 1.1 — Planned), Datacenter (BSL 1.1 — Planned)

---

## Executive Summary

The Production line is **finished through v0.4.0**. The server accepts connections, authenticates users (local/PAM), transfers files in PASV and PORT, supports HOST virtual hosting, FTPS, and **MODE Z** streaming compression when `transfer.enable_compression` is set. Remaining work is Enterprise/Datacenter, optional coverage/load bars, and Linux/Windows/Docker env verification on hosts we do not have here.

### Product Version Status

- **Production Version (Apache 2.0):** complete through v0.4.0
- **Enterprise Version (BSL 1.1):** 0% — Planned
- **Datacenter Version (BSL 1.1):** 0% — Planned

---

## What Works (Production Version)

- Socket server (listen, accept, multi-client)
- FTP command set including USER, PASS, QUIT, PWD, CWD, LIST, RETR, STOR, DELE, MKD, RMD, SIZE, TYPE, NOOP, SYST, FEAT, PORT, EPRT, PASV, MODE, HOST, AUTH/PBSZ/PROT
- Local and PAM authentication (PAM Linux-only)
- File transfers through data connections (passive and active)
- MODE Z streaming zlib on RETR/STOR (REST/APPE rejected while MODE Z is active)
- Virtual hosting (HOST), persistent JSON users, groups, guests, quotas, session limits
- INI/JSON/YAML configuration; STANDARD/JSON/EXTENDED logging
- CLI: start, stop, restart, status, reload, test, user, virtual, ssl
- Test suite: 59 tests (1 skipped PAM on macOS) including protocol integration
- FTPS (OpenSSL), chroot, privilege dropping, IP ACL, rate limiting, bandwidth throttle
- Connection pooling, sendfile/mmap (skipped when MODE Z)

### Pending (not Production polish)

- Broader coverage / load benchmarks (optional 1.0 bar)
- systemd / Docker / Windows / Linux package smoke on those hosts (see [VERIFICATION.md](VERIFICATION.md))
- Enterprise / Datacenter features

---

## Phase 1 environment verification (2026-08-30)

| Check | Result |
|-------|--------|
| `simple-sftpd test --config` (simple INI/JSON/YAML) | Pass |
| launchd plist `plutil -lint` | Pass |
| User LaunchAgent start / 220+QUIT / bootout | Pass |
| System `/Library/LaunchDaemons` install | Not run |
| Docker | Not run (Docker not installed) |
| macOS native CPack `.pkg` | Pass (ldapd-style rebuild) |
| Linux systemd / DEB / RPM | Not run (no Linux host) |
| Windows service | Not run (no Windows host) |

---

## Component status

| Component | Status | Notes |
|-----------|--------|-------|
| Socket / data connections | Complete | PASV + PORT/EPRT dispatched and tested |
| Authentication | Complete | Local + PAM (Linux); guests/expiry |
| File operations | Complete | RETR/STOR/REST/APPE/RNFR/RNTO; MODE Z on RETR/STOR |
| Virtual hosting | Complete | HOST + `virtual` CLI |
| Configuration / logging | Complete | INI/JSON/YAML |
| SSL/TLS | Complete | AUTH TLS smoke in integration tests |
| CLI | Complete | Management + user + virtual + ssl |
| Tests | Green | ~59 tests; protocol integration added in v0.4.0 |
| Docker / Linux packages | Ready in-tree | Not smoke-tested on this Mac |

Approximate unit/integration coverage is still short of a 90% 1.0 bar; v0.4.0 did not block on that.

---

## Timeline

| Version | Status |
|---------|--------|
| Production 0.1.0 | Released 2025-11-27 |
| Production 0.2.0 | Complete |
| Production 0.3.0 | Complete (tagged) |
| Production 0.4.0 | Complete 2026-08-30 (tag when asked) |
| Enterprise / Datacenter | Planned |

---

## Honest Assessment

**Strengths:** Working FTP/FTPS server, PORT actually dispatched, MODE Z wired, protocol tests green, macOS launchd user-agent and config/package smoke recorded.

**Remaining:** Linux/Windows/Docker verification, load/pen-test bars, Enterprise/Datacenter.

**Overall:** Production polish is done. Tag **v0.4.0** when you want the release cut.

---

*Last Updated: August 2026*  
*Next Review: Before Enterprise v0.1.0 kickoff*  
*Focus: Production Version (Apache 2.0) — complete through v0.4.0*
