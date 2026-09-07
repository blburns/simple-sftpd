# Build Guide

Complete guide to building Simple Secure FTP Daemon for different product versions and platforms.

**Current release:** Production v0.4.0

## Quick Start

```bash
# Clone repository
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd

# Build Production version
mkdir build && cd build
cmake -DBUILD_VERSION=production -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run
./simple-sftpd --help
```

Or use the top-level Makefile:

```bash
make deps    # Install platform dependencies (Linux/macOS/FreeBSD)
make build   # Release build via CMake
make test    # Run Google Test suite
```

**FreeBSD:** Requires GNU make (`gmake`). The root `Makefile` forwards to `GNUmakefile`.

---

## Build Commands Reference

### Basic Build

```bash
mkdir build && cd build
cmake -DBUILD_VERSION=production ..
make
sudo make install
```

### Version-Specific Builds

| Product | CMake flag |
|---------|------------|
| Production (default) | `-DBUILD_VERSION=production` |
| Enterprise (stub) | `-DBUILD_VERSION=enterprise` |
| Datacenter (stub) | `-DBUILD_VERSION=datacenter` |

Only **Production** is fully implemented today.

### Common CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_SSL` | ON | FTPS via OpenSSL |
| `ENABLE_JSON` | ON | JSON config parsing (jsoncpp) |
| `ENABLE_COMPRESSION` | ON | Compression library (not yet on wire) |
| `ENABLE_TESTS` | ON | Build and register CTest targets |
| `CMAKE_BUILD_TYPE` | — | `Debug` or `Release` |

PAM and bzip2 are auto-detected on Linux when development packages are installed.

---

## Platform Notes

| Platform | Build | Notes |
|----------|-------|-------|
| Linux (Debian/RHEL) | cmake + make | PAM optional; install `libpam0g-dev` for PAM auth |
| macOS | cmake + make | Homebrew: `cmake openssl jsoncpp` |
| FreeBSD | gmake | Uses `GNUmakefile`; jsoncpp from ports |
| Windows | Visual Studio + CMake | Less frequently verified |

---

## Remote / CI Builds

Ansible playbooks under `automation/ansible/` build on Debian, RHEL, FreeBSD, and macOS VMs. See [automation/README.md](../../automation/README.md).

---

## Testing

```bash
cd build
ctest --output-on-failure
# or from repo root:
make test
```

---

**Last Updated:** May 2026
