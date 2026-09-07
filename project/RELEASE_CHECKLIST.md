# simple-sftpd Production Release Checklist

Use this checklist when preparing a Production release (currently **v0.4.0**).

## Pre-Release

- [x] **Build** – `cmake` + build succeeds on **macOS** (2026-08-30). Linux/FreeBSD not run on this host.
- [x] **Tests** – `ctest --output-on-failure` passes (~59 tests; 1 PAM skip on macOS).
- [x] **Config test** – `simple-sftpd test --config` for sample INI/JSON/YAML (`config/simple/`).
- [x] **Docs** – Production docs reflect current version:
  - [x] [docs/production/README.md](../docs/production/README.md)
  - [x] [docs/production/deployment.md](../docs/production/deployment.md)
  - [x] [docs/production/operations.md](../docs/production/operations.md)
  - [x] [docs/production/performance.md](../docs/production/performance.md)
  - [x] [docs/production/security.md](../docs/production/security.md)
  - [x] [docs/production/troubleshooting.md](../docs/production/troubleshooting.md)
- [x] **CHANGELOG** – [CHANGELOG.md](../CHANGELOG.md) updated with version and date.
- [x] **Version** – `CMakeLists.txt` `project(VERSION)`, `GNUmakefile`/`Makefile` VERSION, and `SIMPLE_SFTPD_VERSION` in `main/production.cpp` match **0.4.0**.

## Release

- [ ] Tag: `git tag -s v0.4.0 -m "Release v0.4.0"` (only when asked).
- [ ] Create GitHub release with notes from CHANGELOG.
- [ ] Publish build artifacts (packages/binaries) if applicable.

## Post-Release

- [x] Update [PROJECT_STATUS.md](PROJECT_STATUS.md) and [PROGRESS_REPORT.md](PROGRESS_REPORT.md) for v0.4.0 (pre-tag).
- [x] Update [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md) for the 0.4.0 milestone.

---

**Version:** 0.4.0  
**Last Updated:** August 2026
