# Simple Secure FTP Daemon Documentation

Welcome to the Simple Secure FTP Daemon documentation. This guide will help you understand, configure, and deploy the Simple Secure FTP Daemon effectively.

## Product Versions

Simple Secure FTP Daemon is available in three versions, each designed for different deployment scenarios:

### 🏭 Production Version
**License:** Apache 2.0  
**Status:** ✅ Feature-complete through v0.3.0 (v0.1.0 released 2025-11-27)  
**Target:** Small to medium deployments, single-server installations

- Complete FTP protocol (RFC 959), passive and active mode
- FTPS (SSL/TLS), PAM, chroot, privilege dropping, IP access control
- Virtual hosting (HOST command), persistent JSON users, groups, guest accounts, quotas
- Multi-format configuration (INI, JSON, YAML) with hot reload (SIGHUP)
- Cross-platform: Linux, macOS, FreeBSD, Windows

**Documentation:** [Production Version Documentation](production/README.md)

### 🏢 Enterprise Version
**License:** BSL 1.1 (Business Source License 1.1)
**Status:** 📋 Planned
**Target:** Large deployments, multi-server environments, enterprise integrations

- All Production Version features included
- Web management interface
- REST API for management
- SNMP integration
- Advanced authentication and authorization (RBAC, ACLs)
- Advanced security (rate limiting, DDoS protection)
- High availability and clustering
- Advanced monitoring and observability
- Plugin architecture

**Documentation:** [Enterprise Version Documentation](enterprise/README.md)

### 🏛️ Datacenter Version
**License:** BSL 1.1 (Business Source License 1.1)
**Status:** 📋 Planned
**Target:** Large-scale datacenter deployments, cloud environments, multi-site operations

- All Enterprise Version features included
- Horizontal scaling support
- Multi-site synchronization
- Cloud service integrations
- Advanced performance optimizations
- Multi-tenant support
- Advanced analytics

**Documentation:** [Datacenter Version Documentation](datacenter/README.md)

---

## Documentation Structure

### 📚 Shared Documentation
Common documentation applicable to all versions:

- **[Getting Started](shared/getting-started/)** - Installation and quick start guides
- **[Configuration](shared/configuration/)** - Configuration reference and examples
- **[Diagrams](shared/diagrams/)** - Architecture and flow diagrams
- **[User Guide](shared/user-guide/)** - User documentation
- **[Examples](shared/examples/)** - Usage examples
- **[API Reference](shared/api/)** - API documentation

### 🏭 Production Version Documentation
- **[Production Guide](production/README.md)** - Complete Production Version documentation
- **[Installation](production/installation.md)** - Production installation guide
- **[Configuration](production/configuration.md)** - Production configuration reference
- **[Deployment](production/deployment.md)** - Production deployment guide
- **[Security](production/security.md)** - Production security best practices
- **[Performance](production/performance.md)** - Production performance tuning
- **[Operations](production/operations.md)** - Production operations guide

### 🏢 Enterprise Version Documentation
- **[Enterprise Guide](enterprise/README.md)** - Complete Enterprise Version documentation
- **[Installation](enterprise/installation.md)** - Enterprise installation guide
- **[Management Interface](enterprise/management.md)** - Web UI and REST API
- **[High Availability](enterprise/high-availability.md)** - HA setup and configuration
- **[Integrations](enterprise/integrations.md)** - SNMP and other integrations
- **[Security](enterprise/security.md)** - Advanced security features
- **[Operations](enterprise/operations.md)** - Enterprise operations guide

### 🏛️ Datacenter Version Documentation
- **[Datacenter Guide](datacenter/README.md)** - Complete Datacenter Version documentation
- **[Installation](datacenter/installation.md)** - Datacenter installation guide
- **[Multi-Site Setup](datacenter/multi-site.md)** - Multi-site configuration
- **[Cloud Deployment](datacenter/cloud.md)** - Cloud deployment guides
- **[Scaling](datacenter/scaling.md)** - Horizontal scaling and load balancing
- **[Monitoring](datacenter/monitoring.md)** - Advanced monitoring and analytics

### 👨‍💻 Developer Documentation
Documentation for developers and contributors:

- **[Developer Guide](development/README.md)** - Complete developer documentation
- **[Build Guide](development/BUILD_GUIDE.md)** - Build commands and reference
- **[Setup Guide](development/SETUP.md)** - Development environment setup

---

## Quick Start

### Production Version
1. [Install Simple Secure FTP Daemon](shared/getting-started/README.md)
2. [Quick Start Guide](shared/getting-started/quick-start.md)
3. [Production Configuration](production/configuration.md)
4. [Production Deployment](production/deployment.md)

### Enterprise Version
1. [Install Simple Secure FTP Daemon](shared/getting-started/README.md)
2. [Enterprise Setup](enterprise/README.md)
3. [Management Interface](enterprise/management.md)
4. [High Availability](enterprise/high-availability.md)

### Datacenter Version
1. [Install Simple Secure FTP Daemon](shared/getting-started/README.md)
2. [Datacenter Setup](datacenter/README.md)
3. [Multi-Site Configuration](datacenter/multi-site.md)
4. [Scaling Guide](datacenter/scaling.md)

---

## Version Comparison

| Feature | Production | Enterprise | Datacenter |
|---------|-----------|------------|------------|
| **License** | Apache 2.0 | BSL 1.1 | BSL 1.1 |
| **FTP Protocol** | ✅ Complete | ✅ Complete | ✅ Complete |
| **FTPS (SSL/TLS)** | ✅ | ✅ | ✅ |
| **Basic Security** | ✅ | ✅ | ✅ |
| **Multi-Format Config** | ✅ | ✅ | ✅ |
| **Hot Reload (SIGHUP)** | ✅ | ✅ | ✅ |
| **Virtual Hosting** | ✅ | ✅ | ✅ |
| **Persistent Users / Groups** | ✅ | ✅ | ✅ |
| **Web Management** | ❌ | ✅ | ✅ |
| **REST API** | ❌ | ✅ | ✅ |
| **SNMP Integration** | ❌ | ✅ | ✅ |
| **Authentication/RBAC** | ✅ Basic | ✅ Advanced | ✅ Advanced |
| **Rate Limiting** | ✅ Basic | ✅ Advanced | ✅ Advanced |
| **High Availability** | ❌ | ✅ | ✅ |
| **Clustering** | ❌ | ✅ | ✅ |
| **Horizontal Scaling** | ❌ | ❌ | ✅ |
| **Multi-Site Sync** | ❌ | ❌ | ✅ |
| **Cloud Integration** | ❌ | ❌ | ✅ |
| **Plugin System** | ❌ | ✅ | ✅ |
| **Multi-Tenant** | ❌ | ❌ | ✅ |

---

## Documentation by Topic

### Getting Started
- [Installation Guide](shared/getting-started/README.md) - Install on Linux, macOS, Windows
- [Quick Start](shared/getting-started/quick-start.md) - Get running in minutes
- [First Steps](shared/getting-started/first-steps.md) - Basic configuration

### Configuration
- [Configuration Reference](shared/configuration/README.md) - Complete configuration guide
- [Production Configuration](production/configuration.md) - Production-specific configuration
- [Enterprise Configuration](enterprise/configuration.md) - Enterprise-specific configuration
- [Datacenter Configuration](datacenter/configuration.md) - Datacenter-specific configuration

### Deployment
- [Production Deployment](production/deployment.md) - Production deployment guide
- [Enterprise Deployment](enterprise/deployment.md) - Enterprise deployment guide
- [Datacenter Deployment](datacenter/deployment.md) - Datacenter deployment guide
- [Docker Deployment](shared/deployment/docker.md) - Containerized deployment
- [High Availability](enterprise/high-availability.md) - HA setup (Enterprise+)

### Operations
- [Production Operations](production/operations.md) - Production operations guide
- [Monitoring](shared/deployment/monitoring.md) - Monitoring setup
- [Backup Procedures](shared/deployment/backup-procedures.md) - Backup and restore
- [Maintenance](shared/deployment/maintenance-procedures.md) - Maintenance procedures

### Security
- [Production Security](production/security.md) - Production security best practices
- [Enterprise Security](enterprise/security.md) - Advanced security features
- [Security Best Practices](shared/user-guide/security-best-practices.md) - General security guide

### Performance
- [Production Performance](production/performance.md) - Production performance tuning
- [Enterprise Performance](enterprise/performance.md) - Enterprise performance optimization
- [Datacenter Performance](datacenter/performance.md) - Datacenter scaling and optimization

### Troubleshooting
- [Common Issues](shared/troubleshooting/README.md) - Troubleshooting guide
- [Debugging](shared/troubleshooting/debugging.md) - Debugging techniques
- [Performance Issues](shared/troubleshooting/performance.md) - Performance troubleshooting

---

## Contributing to Documentation

We welcome contributions to improve this documentation. Please see our [Contributing Guide](../CONTRIBUTING.md) for details on how to contribute.

## Feedback

If you find any issues with the documentation or have suggestions for improvement, please:

1. Open an issue on [GitHub](https://github.com/blburns/simple-sftpd/issues)
2. Submit a pull request with your improvements
3. Contact us at docs@simpledaemons.com

---

**Last Updated:** May 2026  
**Production Version:** v0.3.0 (feature-complete)  
**Enterprise Version:** Planned  
**Datacenter Version:** Planned
