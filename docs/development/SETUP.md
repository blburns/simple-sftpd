# Developer Setup Guide

Step-by-step guide to set up your development environment for Simple Secure FTP Daemon.

**Current version:** Production v0.4.0

## Prerequisites Checklist

- [ ] C++17 compiler (GCC 7+, Clang 8+, or MSVC 2017+)
- [ ] CMake 3.16+
- [ ] Git
- [ ] OpenSSL development libraries
- [ ] jsoncpp development libraries
- [ ] pkg-config (Linux/macOS)

---

## Step 1: Install System Dependencies

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libjsoncpp-dev \
    libpam0g-dev \
    pkg-config
```

### Linux (RHEL/CentOS/Fedora)

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake openssl-devel jsoncpp-devel pam-devel
```

### macOS

```bash
brew install cmake openssl jsoncpp
```

### FreeBSD

```bash
pkg install cmake gmake openssl jsoncpp
# Build with: gmake build
```

---

## Step 2: Clone Repository

```bash
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd
```

---

## Step 3: Initial Build

```bash
make deps          # optional: install deps via make
make build         # Release build
make test          # run tests
```

Or with CMake directly:

```bash
mkdir build && cd build
cmake -DBUILD_VERSION=production -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
ctest --output-on-failure
```

---

## Project Layout

Source lives under `include/simple-sftpd/` and `src/simple-sftpd/` (core, config, user, virtual_host, security, utils). Entry point: `main/production.cpp`.

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for platform-specific notes and CMake options.

---

**Last Updated:** May 2026
