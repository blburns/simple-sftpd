# simple-sftpd Production Release Checklist

Use this checklist when preparing a Production release (currently **v0.3.0**).

## Pre-Release

- [ ] **Build** – `cmake -DBUILD_VERSION=production .. && make` succeeds on Linux and macOS (and FreeBSD via `gmake` if targeting).
- [ ] **Tests** – `ctest --output-on-failure` passes (51 unit/integration tests).
- [ ] **Config test** – `simple-sftpd test --config <path>` runs without error for sample INI/JSON/YAML configs.
- [ ] **Docs** – Production docs reflect current version:
  - [ ] [docs/production/README.md](../docs/production/README.md)
  - [ ] [docs/production/deployment.md](../docs/production/deployment.md)
  - [ ] [docs/production/operations.md](../docs/production/operations.md)
  - [ ] [docs/production/performance.md](../docs/production/performance.md)
  - [ ] [docs/production/security.md](../docs/production/security.md)
  - [ ] [docs/production/troubleshooting.md](../docs/production/troubleshooting.md)
- [ ] **CHANGELOG** – [CHANGELOG.md](../CHANGELOG.md) updated with version and date.
- [ ] **Version** – `CMakeLists.txt` `project(VERSION)`, `GNUmakefile`/`Makefile` VERSION, and `SIMPLE_SFTPD_VERSION` in `main/production.cpp` match.

## Release

- [ ] Tag: `git tag -s v0.3.0 -m "Release v0.3.0"` (adjust version as needed).
- [ ] Create GitHub release with notes from CHANGELOG.
- [ ] Publish build artifacts (packages/binaries) if applicable.

## Post-Release

- [ ] Update [PROJECT_STATUS.md](PROJECT_STATUS.md) and [PROGRESS_REPORT.md](PROGRESS_REPORT.md).
- [ ] Update [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) if milestone boundaries changed.

---

**Version:** 0.3.0  
**Last Updated:** May 2026
