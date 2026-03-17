# Source Code Structure Proposal

## Current Structure

```
simple-sftpd/
├── include/simple-sftpd/
│   └── [various headers]
├── src/simple-sftpd/  # Implementation
└── CMakeLists.txt     # Single build configuration
```

## Proposed Structure

### Modular Directory Structure (Recommended)

```
simple-sftpd/
├── src/
│   ├── core/                    # Shared core (Production base)
│   │   ├── ftp/
│   │   ├── config/
│   │   └── utils/
│   ├── production/              # Production-specific features
│   ├── enterprise/              # Enterprise-specific features
│   └── datacenter/              # Datacenter-specific features
├── include/
│   └── simple-sftpd/
│       ├── core/
│       ├── production/
│       ├── enterprise/
│       └── datacenter/
├── main/
│   ├── production.cpp
│   ├── enterprise.cpp
│   └── datacenter.cpp
└── CMakeLists.txt               # Version-aware build
```

## Build Commands

```bash
# Build Production
cmake -DBUILD_VERSION=production ..
make

# Build Enterprise
cmake -DBUILD_VERSION=enterprise ..
make

# Build Datacenter
cmake -DBUILD_VERSION=datacenter ..
make
```
