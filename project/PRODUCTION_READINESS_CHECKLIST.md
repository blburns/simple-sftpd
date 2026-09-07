# Production Readiness Checklist

Short, actionable checklist to get simple-sftpd to production level. See [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) for the full roadmap and [PROGRESS_REPORT.md](PROGRESS_REPORT.md) for detailed status.

**Deliverables added (codebase):** Phase 1 fixes (launchd path, install script `Type=simple`, Docker default config), [VERIFICATION.md](VERIFICATION.md) (steps for 1–8), [docs/TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md), [docs/PRODUCTION_DEPLOYMENT.md](../docs/PRODUCTION_DEPLOYMENT.md), [docs/MIGRATION_GUIDE.md](../docs/MIGRATION_GUIDE.md), [SECURITY_CHECKLIST.md](SECURITY_CHECKLIST.md), [.github/workflows/ci.yml](../.github/workflows/ci.yml) (Linux/macOS/Windows + Docker + static analysis), [.github/workflows/release.yml](../.github/workflows/release.yml), [scripts/load-test.sh](../scripts/load-test.sh), unit test for compression. Items 1–8 still require running in real/VM environments.

---

## Phase 1: Environment verification (close the 5% gap)

Prove the server and packaging work in real or VM environments. **Steps:** [VERIFICATION.md](VERIFICATION.md).

| # | Task | Owner | Done |
|---|------|-------|------|
| 1 | Run server under **systemd** (Linux): start, stop, restart, logs, PID file | | [ ] not run — no Linux host; CI-aspirational |
| 2 | Run server under **launchd** (macOS): same checks | | [x] user LaunchAgent smoke 2026-08-30 (not system LaunchDaemon) |
| 3 | Run server as **Windows service**: same checks | | [ ] not run — no Windows host |
| 4 | Test **install scripts**: `tools/install-service.sh`, `etc/windows/install-service.bat` | | [ ] not run — would require system install |
| 5 | **Linux build**: configure, build, run smoke test (login + one transfer) | | [ ] not run here; CI-aspirational |
| 6 | **Windows build**: same (or document “unsupported” if deferred) | | [ ] deferred — no Windows host |
| 7 | **Docker** | | [ ] not used — native CPack packaging (same as simple-ldapd) |
| 8 | **Packaging**: native CPack (macOS PKG/DMG, Linux DEB/RPM) | | [x] macOS PKG rebuilt 2026-08-30 (Linux DEB/RPM not built here) |

---

## Phase 2: Ops and CI (deployable and supportable)

| # | Task | Owner | Done |
|---|------|-------|------|
| 9 | **Troubleshooting guide**: common errors, log locations, config checks, how to get help | | [x] |
| 10 | **Production deployment / best-practices** doc: hardening, TLS, users, chroot, logging | | [x] |
| 11 | **CI pipeline**: build + run tests on at least Linux and macOS (e.g. GitHub Actions) | | [x] |

---

## Phase 3: Toward 1.0 “Production ready” (roadmap bar)

Optional for first production use; required for the roadmap’s Version 1.0.0 bar.

### Quality and security

| # | Task | Owner | Done |
|---|------|-------|------|
| 12 | **Test coverage** toward 90%+ (current ~40%) | | [ ] |
| 13 | **Load testing**: many concurrent connections and transfers | | [x] |
| 14 | **Performance benchmarks**: define and meet throughput/latency targets | | [ ] |
| 15 | **Security review**: internal or third-party | | [ ] |
| 16 | **Penetration testing**: FTP/FTPS and auth focus | | [ ] |
| 17 | **Cross-platform testing**: Linux, macOS, Windows in CI or release process | | [ ] |

### Documentation and process

| # | Task | Owner | Done |
|---|------|-------|------|
| 18 | **Complete documentation**: all features and config options | | [ ] |
| 19 | **Migration / upgrade guide** for future version bumps | | [x] |
| 20 | **Static analysis** in CI (e.g. clang-tidy, cppcheck) | | [x] |
| 21 | **Release automation**: version tagging, changelog, artifact build | | [x] |

---

## v0.2.0 / v0.3.0 enhancements

| # | Task | Owner | Done |
|---|------|-------|------|
| 22 | Connection pooling (replace or augment thread-per-connection) | | [x] |
| 23 | Memory-mapped I/O / sendfile for large file transfers | | [x] |
| 24 | Integrate compression into RETR/STOR (MODE Z, v0.4.0) | | [x] |
| 25 | Virtual hosting (HOST routing, per-host config/SSL/quotas) | | [x] |
| 26 | Persistent user storage (JSON file, auto load/save) | | [x] |
| 27 | Advanced user management (groups, quotas, sessions, guest accounts) | | [x] |

---

## Definition of done

- **Production level (Phase 1 + 2):** All Phase 1 and Phase 2 items checked; server runs and is supportable in at least one real or VM environment with docs and CI.
- **1.0 Production ready:** Phase 1–3 complete per roadmap; quality, security, and documentation bars met.

---

See [VERIFICATION.md](VERIFICATION.md) for the 2026-08-30 Phase 1 results table.

*Last updated: August 2026*
