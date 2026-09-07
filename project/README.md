# Project Management Documentation

This directory contains project management, development, and internal documentation for the Simple Secure FTP Daemon project.

## 📦 Product Versions

The Simple Secure FTP Daemon is organized into three product versions:

- **🏭 Production Version (Apache 2.0)** - Basic FTP server, CLI management, single-server deployments
- **🏢 Enterprise Version (BSL 1.1)** - All Production features + Web UI, REST API, SNMP, HA, clustering
- **🏛️ Datacenter Version (BSL 1.1)** - All Enterprise features + Horizontal scaling, multi-site sync, cloud integrations

**Note:** Most documentation in this directory focuses on the **Production Version**, which is complete through v0.4.0.

## 📋 Document Organization

### Project Status & Progress
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)** - Overall project status, completion metrics, and health indicators (Production Version focus)
- **[PROGRESS_REPORT.md](PROGRESS_REPORT.md)** - Detailed progress report with honest assessment of what works and what's pending (Production Version focus)

### Implementation & Features
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Summary of recent improvements and feature completions (Production Version)
- **[FEATURE_AUDIT.md](FEATURE_AUDIT.md)** - Comprehensive audit of implemented vs. stubbed features (Production Version)

### Planning & Roadmap
- **[ROADMAP.md](../ROADMAP.md)** - Development roadmap and future plans (in project root for visibility)
- **[ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md)** - Detailed checklist tracking roadmap items by product version (Production, Enterprise, Datacenter)

### Technical Debt
- **[TECHNICAL_DEBT.md](TECHNICAL_DEBT.md)** - Technical debt, known issues, and areas requiring improvement (Production Version)

### Release & Readiness
- **[../PROJECT_OVERVIEW.md](../PROJECT_OVERVIEW.md)** - Short executive overview
- **[RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md)** - Steps for preparing a production release
- **[../RELEASING.md](../RELEASING.md)** - Tag and publish how-to
- **[../VERSIONING.md](../VERSIONING.md)** - SemVer policy
- **[PRODUCTION_READINESS_CHECKLIST.md](PRODUCTION_READINESS_CHECKLIST.md)** - Actionable checklist to reach production readiness
- **[VERIFICATION.md](VERIFICATION.md)** - How to verify service, build, Docker, and packaging in real/VM environments
- **[SECURITY_CHECKLIST.md](SECURITY_CHECKLIST.md)** - Security checklist for deployment and review

## 📚 User Documentation

For user-facing documentation (installation, configuration, usage guides), see the **[docs/](../docs/)** directory.

## 📝 Project History

For version history and changes, see **[CHANGELOG.md](../CHANGELOG.md)** in the project root.

*Doc set standard: SimpleDaemons `STANDARDIZATION_TEMPLATES/docs/PROJECT_DOCS_STANDARD.md`*

---

*This directory is for internal project management. User documentation is located in the `docs/` directory.*

