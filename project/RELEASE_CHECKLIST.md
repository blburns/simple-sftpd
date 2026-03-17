# simple-sftpd Production Release Checklist

Use this checklist when preparing a production release (e.g. v0.1.0).

## Pre-Release

- [ ] **Build** – `cmake .. && make` succeeds with default options (Linux/macOS).
- [ ] **Tests** – `ctest --output-on-failure` passes (all unit and integration tests).
- [ ] **Config test** – `simple-sftpd test --config <path>` runs without error for at least one sample config (INI/JSON/YAML).
- [ ] **Docs** – Production docs are current:
  - [ ] [docs/production/README.md](../docs/production/README.md)
  - [ ] [docs/production/deployment.md](../docs/production/deployment.md)
  - [ ] [docs/production/operations.md](../docs/production/operations.md)
  - [ ] [docs/production/performance.md](../docs/production/performance.md)
  - [ ] [docs/production/security.md](../docs/production/security.md)
  - [ ] [docs/production/troubleshooting.md](../docs/production/troubleshooting.md)
- [ ] **CHANGELOG** – [CHANGELOG.md](../CHANGELOG.md) updated with version and date.
- [ ] **Version** – CMake `project(simple-sftpd VERSION x.y.z)` and any version strings in docs match release.

## Release

- [ ] Tag: `git tag -s v0.1.0 -m "Release v0.1.0"` (or equivalent).
- [ ] Create GitHub (or repo) release with release notes from CHANGELOG.
- [ ] Attach or publish build artifacts (packages, binaries) if applicable.

## Post-Release

- [ ] Update [PROJECT_STATUS.md](PROJECT_STATUS.md) to “Released” and set date.
- [ ] Update [PROGRESS_REPORT.md](PROGRESS_REPORT.md) if maintained.
- [ ] Announce (internal/wiki/mail as appropriate).

---

**Version:** 0.1.0  
**Last Updated:** March 2025
