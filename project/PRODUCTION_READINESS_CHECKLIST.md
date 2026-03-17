# Production Readiness Checklist

Short, actionable checklist to get simple-sftpd to production level. See [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) for the full roadmap and [PROGRESS_REPORT.md](PROGRESS_REPORT.md) for detailed status.

**Deliverables added (codebase):** Phase 1 fixes (launchd path, install script `Type=simple`, Docker default config), [VERIFICATION.md](VERIFICATION.md) (steps for 1–8), [docs/TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md), [docs/PRODUCTION_DEPLOYMENT.md](../docs/PRODUCTION_DEPLOYMENT.md), [docs/MIGRATION_GUIDE.md](../docs/MIGRATION_GUIDE.md), [SECURITY_CHECKLIST.md](SECURITY_CHECKLIST.md), [.github/workflows/ci.yml](../.github/workflows/ci.yml) (Linux/macOS/Windows + Docker + static analysis), [.github/workflows/release.yml](../.github/workflows/release.yml), [scripts/load-test.sh](../scripts/load-test.sh), unit test for compression. Items 1–8 still require running in real/VM environments.

---

## Phase 1: Environment verification (close the 5% gap)

Prove the server and packaging work in real or VM environments. **Steps:** [VERIFICATION.md](VERIFICATION.md).

| # | Task | Owner | Done |
|---|------|-------|------|
| 1 | Run server under **systemd** (Linux): start, stop, restart, logs, PID file | | [ ] |
| 2 | Run server under **launchd** (macOS): same checks | | [ ] |
| 3 | Run server as **Windows service**: same checks | | [ ] |
| 4 | Test **install scripts**: `tools/install-service.sh`, `etc/windows/install-service.bat` | | [ ] |
| 5 | **Linux build**: configure, build, run smoke test (login + one transfer) | | [ ] |
| 6 | **Windows build**: same (or document “unsupported” if deferred) | | [ ] |
| 7 | **Docker**: build image, run container, test FTP (control + data) and config mount | | [ ] |
| 8 | **Packaging**: build at least one package (DEB or RPM), install, run, basic transfer test | | [ ] |

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

## Optional v0.2.0 enhancements (not blocking production)

| # | Task | Owner | Done |
|---|------|-------|------|
| 22 | Connection pooling (replace or augment thread-per-connection) | | [ ] |
| 23 | Memory-mapped I/O for large file transfers | | [ ] |
| 24 | Integrate compression into RETR/STOR (class exists, not wired) | | [ ] |

---

## Definition of done

- **Production level (Phase 1 + 2):** All Phase 1 and Phase 2 items checked; server runs and is supportable in at least one real or VM environment with docs and CI.
- **1.0 Production ready:** Phase 1–3 complete per roadmap; quality, security, and documentation bars met.

---

*Last updated: February 2025*
