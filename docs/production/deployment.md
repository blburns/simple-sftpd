# Production Version Deployment Guide

**Version:** 0.1.0  
**License:** Apache 2.0

---

## Overview

This guide covers deploying the Production Version of Simple Secure FTP Daemon (simple-sftpd) in production environments.

## Pre-Deployment Checklist

- [ ] System requirements met (see README / BUILD_GUIDE)
- [ ] Network and firewall planned (FTP control port 21, passive port range)
- [ ] Configuration file prepared and validated
- [ ] SSL/TLS certificates ready (for FTPS)
- [ ] User accounts or PAM configured
- [ ] Log directory and rotation configured
- [ ] Service/process manager configured (systemd, launchd, etc.)

## Deployment Methods

### Build from Source

```bash
cd simple-sftpd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SSL=ON -DENABLE_TESTS=ON
make -j$(nproc)
sudo make install
```

### Package Installation (when packages are available)

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install simple-sftpd
```

**CentOS/RHEL:**
```bash
sudo yum install simple-sftpd
```

### Docker Deployment

```bash
docker run -d \
  --name simple-sftpd \
  -p 21:21 \
  -p 49152-65535:49152-65535 \
  -v /var/ftp:/var/ftp \
  -v /etc/simple-sftpd:/etc/simple-sftpd \
  -v /var/log/simple-sftpd:/var/log/simple-sftpd \
  simpledaemons/simple-sftpd:0.1.0
```

Ensure the passive port range (e.g. 49152-65535) is exposed or use a fixed range and map it.

## Configuration

1. **Create directories**
```bash
sudo mkdir -p /etc/simple-sftpd /var/log/simple-sftpd /var/ftp
sudo chown ftp:ftp /var/ftp
```

2. **Install or create configuration**
   - Use `config/production/simple-sftpd.yml` or equivalent as a template.
   - Support formats: INI (`.conf`), JSON (`.json`), YAML (`.yml`/`.yaml`).

3. **Validate configuration**
```bash
simple-sftpd test --config /etc/simple-sftpd/simple-sftpd.conf
```

4. **SSL/TLS (FTPS)**  
   - Place certificate and key in e.g. `/etc/simple-sftpd/ssl/`.
   - Set `ssl.enabled`, `ssl.certificate_file`, `ssl.private_key_file` in config.
   - Use `tools/setup-ssl.sh` or your CA to generate certs.

## Network Configuration

### Firewall

- **Control channel:** TCP 21 (FTP).
- **Passive data ports:** TCP range configured in `passive.min_port`–`passive.max_port` (e.g. 49152–65535).

**UFW (Ubuntu):**
```bash
sudo ufw allow 21/tcp
sudo ufw allow 49152:65535/tcp
sudo ufw reload
```

**firewalld (CentOS/RHEL):**
```bash
sudo firewall-cmd --permanent --add-service=ftp
sudo firewall-cmd --permanent --add-port=49152-65535/tcp
sudo firewall-cmd --reload
```

### Passive mode

- Set `passive.external_ip` if clients connect via NAT and need the external IP in PASV responses.
- Ensure the passive port range is open on any NAT/firewall between clients and the server.

## Service Setup

### systemd (Linux)

Copy or link the service file:
```bash
sudo cp deployment/systemd/simple-sftpd.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable simple-sftpd
sudo systemctl start simple-sftpd
```

### launchd (macOS)

Install the plist to `/Library/LaunchDaemons/` and load as needed (see `deployment/launchd/`).

## Post-Deployment

- Run `simple-sftpd status` and confirm the process is listening.
- Test login and a small upload/download with an FTP client (passive and, if used, active).
- Confirm log output and log rotation.
- Document your config paths, port range, and any custom security settings for operations and troubleshooting.

---

**Last Updated:** March 2025  
**Version:** 0.1.0
