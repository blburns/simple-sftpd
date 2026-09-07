# Simple Secure FTP Daemon (simple-sftpd)

**Production v0.4.0** · Apache 2.0

A secure, configurable FTP server written in C++17 for Linux, macOS, FreeBSD, and Windows.

## Features

### Core FTP Functionality
- **RFC 959 Compliant**: Full FTP protocol implementation
- **Passive Mode**: Full PASV support with data connections
- **Active Mode**: Full PORT command support
- **File Transfer**: Upload, download, append, and resume support
- **Directory Operations**: List, create, remove, and navigate directories
- **File Management**: Delete, rename, and directory operations

### Security Features
- **SSL/TLS Support**: Full FTPS implementation with OpenSSL
- **User Authentication**: Username/password and PAM authentication
- **Access Control**: Basic permissions, path restrictions, and IP whitelisting
- **Path Validation**: Directory traversal protection and home directory enforcement
- **Chroot Support**: Directory isolation and privilege dropping
- **Rate Limiting**: Time-window based protection against abuse and DoS attacks
- **Bandwidth Throttling**: Upload and download speed limits

### Virtual Hosting
- **Multi-Domain Support**: Host multiple FTP sites on one server
- **Per-Host Configuration**: Individual settings for each virtual host
- **SSL Certificate Management**: Separate certificates per domain
- **Access Control**: User restrictions per virtual host

### User Management
- **Flexible Permissions**: Granular control over user capabilities
- **Anonymous Access**: Configurable anonymous user support
- **Guest Accounts**: Limited access for temporary users
- **Connection Limits**: Per-user connection and transfer restrictions
- **Session Management**: Timeout and activity tracking

### Performance & Monitoring
- **Multi-threaded**: Efficient handling of multiple connections
- **Connection Management**: Thread-safe connection tracking and cleanup
- **Transfer Optimization**: sendfile and memory-mapped I/O for downloads (Linux/macOS)
- **Statistics**: Performance monitor and file metadata cache
- **Logging**: Advanced logging with STANDARD, JSON, and EXTENDED formats

### Platform Support
- **Cross-Platform**: Linux, macOS, FreeBSD, and Windows
- **Native Builds**: Optimized for each platform
- **Package Management**: DEB, RPM, PKG, and MSI packages
- **Service Integration**: systemd, launchd, and Windows services

## Quick Start

### 🐳 Docker (Recommended)

The fastest way to get started with simple-sftpd is using Docker:

```bash
# Clone the repository
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd

# Quick start with Docker
cd deployment/examples/docker
docker-compose up -d

# Test the FTP service
nc -z localhost 21
```

**Docker Features:**
- ✅ **Zero dependencies** - No need to install build tools
- ✅ **Cross-platform** - Works on Linux, macOS, Windows
- ✅ **Production-ready** - Optimized runtime image
- ✅ **Development environment** - Full debugging tools included
- ✅ **Multi-architecture** - x86_64, ARM64, ARMv7 support

For detailed Docker deployment, see [Docker Deployment Guide](docs/shared/deployment/docker.md).

### Traditional Installation

#### Prerequisites

- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake 3.16+**: Build system
- **OpenSSL**: SSL/TLS support
- **jsoncpp**: JSON configuration parsing

#### From Source

```bash
# Clone the repository
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd

# Build the project
make install-dev  # Install development dependencies
make build        # Build the application
make install      # Install system-wide
```

#### From Packages

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install simple-sftpd
```

**CentOS/RHEL:**
```bash
sudo yum install simple-sftpd
# or
sudo dnf install simple-sftpd
```

**macOS:**
```bash
brew install simple-sftpd
```

### Configuration

simple-sftpd supports multiple configuration formats (INI, JSON, YAML) and provides example configurations for different use cases:

**Configuration Formats:**
- **INI** (`.conf`) - Traditional format
- **JSON** (`.json`) - Machine-readable format
- **YAML** (`.yml`, `.yaml`) - Human-readable format

Format is detected automatically from the file extension.

**Example Configurations:**
- `config/simple/` - Minimal configuration for basic setups
- `config/advanced/` - Enhanced configuration with SSL/TLS and performance tuning
- `config/production/` - Hardened configuration for production deployments

1. **Choose and copy a configuration:**
```bash
# For simple setup
sudo cp config/simple/simple-sftpd.conf /etc/simple-sftpd/simple-sftpd.conf

# For advanced setup
sudo cp config/advanced/simple-sftpd.conf /etc/simple-sftpd/simple-sftpd.conf

# For production
sudo cp config/production/simple-sftpd.conf /etc/simple-sftpd/simple-sftpd.conf
```

2. **Edit the configuration file:**
```bash
sudo nano /etc/simple-sftpd/simple-sftpd.conf
```

3. **Create necessary directories:**
```bash
sudo mkdir -p /var/ftp /var/log/simple-sftpd
sudo chown ftp:ftp /var/ftp
```

For detailed configuration options, see [Configuration Guide](docs/shared/configuration/README.md).

### Running the Server

#### Basic Usage

```bash
# Start in foreground (for testing)
sudo simple-sftpd start --config /etc/simple-sftpd/simple-sftpd.conf

# Start as daemon
sudo simple-sftpd --daemon start

# Test configuration
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf
```

#### Service Management

**Linux (systemd):**
```bash
sudo systemctl enable simple-sftpd
sudo systemctl start simple-sftpd
sudo systemctl status simple-sftpd
```

**macOS (launchd):**
```bash
sudo launchctl load /Library/LaunchDaemons/com.blburns.simple-sftpd.plist
sudo launchctl start com.blburns.simple-sftpd
```

**Windows:**
```cmd
sc create simple-sftpd binPath= "C:\Program Files\simple-sftpd\bin\simple-sftpd.exe"
sc start simple-sftpd
```

## Configuration

### Main Configuration File

The main configuration file (`simple-sftpd.conf`) supports both INI and JSON formats. Here's an example INI configuration:

```ini
# Global server settings
server_name = "Simple-Secure FTP Daemon"
server_version = "0.4.0"
enable_ssl = true
enable_virtual_hosts = true

# SSL Configuration
[ssl]
enabled = true
certificate_file = "/etc/simple-sftpd/ssl/server.crt"
private_key_file = "/etc/simple-sftpd/ssl/server.key"

# Connection settings
[connection]
bind_address = "0.0.0.0"
bind_port = 21
max_connections = 100

# Virtual hosts
[virtual_hosts.default]
hostname = "default"
document_root = "/var/ftp"
enabled = true

# Users
[users.admin]
username = "admin"
password_hash = "$2y$10$hashed_password"
home_directory = "/var/ftp/admin"
permissions = ["READ", "WRITE", "LIST", "UPLOAD", "DOWNLOAD"]
```

### User Management

#### Adding Users

```bash
# Add a new user
sudo simple-sftpd user add \
  --username john \
  --password secret \
  --home /var/ftp/john \
  --permissions READ,WRITE,LIST,UPLOAD,DOWNLOAD

# Add anonymous user
sudo simple-sftpd user add \
  --username anonymous \
  --home /var/ftp/public \
  --anonymous \
  --permissions READ,LIST,DOWNLOAD
```

#### User Permissions

Available permissions:
- `READ`: Read files and directories
- `WRITE`: Write/create files and directories
- `DELETE`: Delete files and directories
- `RENAME`: Rename files and directories
- `MKDIR`: Create directories
- `RMDIR`: Remove directories
- `LIST`: List directory contents
- `UPLOAD`: Upload files
- `DOWNLOAD`: Download files
- `APPEND`: Append to files
- `ADMIN`: Administrative operations

### Virtual Hosts

#### Adding Virtual Hosts

```bash
# Add a new virtual host
sudo simple-sftpd virtual add \
  --hostname ftp.example.com \
  --root /var/ftp/example \
  --ssl \
  --certificate /etc/ssl/certs/example.com.crt \
  --private-key /etc/ssl/private/example.com.key
```

#### Virtual Host Configuration

Each virtual host can have:
- Separate document root
- Individual SSL certificates
- Custom security settings
- User access restrictions
- Transfer rate limits

### SSL/TLS Configuration

#### Generating Self-Signed Certificates

```bash
# Generate certificate for a domain
sudo simple-sftpd ssl generate \
  --hostname ftp.example.com \
  --country US \
  --state California \
  --city San Francisco \
  --organization "Example Corp" \
  --email admin@example.com
```

#### Installing Certificates

```bash
# Install existing certificates
sudo simple-sftpd ssl install \
  --hostname ftp.example.com \
  --certificate /path/to/certificate.crt \
  --private-key /path/to/private.key \
  --ca-certificate /path/to/ca.crt
```

## Command Line Interface

### Server Commands

```bash
# Start the server
simple-sftpd start [--config FILE] [--daemon] [--foreground]

# Stop the server
simple-sftpd stop

# Restart the server
simple-sftpd restart

# Show server status
simple-sftpd status

# Reload configuration
simple-sftpd reload
```

### User Management Commands

```bash
# List all users
simple-sftpd user list

# Add user
simple-sftpd user add --username NAME --password PASS --home DIR

# Remove user
simple-sftpd user remove --username NAME

# Modify user
simple-sftpd user modify --username NAME --permissions READ,WRITE

# Change password
simple-sftpd user password --username NAME --password NEW_PASS
```

### Virtual Host Commands

```bash
# List virtual hosts
simple-sftpd virtual list

# Add virtual host
simple-sftpd virtual add --hostname DOMAIN --root DIR

# Remove virtual host
simple-sftpd virtual remove --hostname DOMAIN

# Enable/disable virtual host
simple-sftpd virtual enable --hostname DOMAIN
simple-sftpd virtual disable --hostname DOMAIN
```

### SSL Management Commands

```bash
# Generate certificate
simple-sftpd ssl generate --hostname DOMAIN

# Install certificate
simple-sftpd ssl install --hostname DOMAIN --cert FILE --key FILE

# Show SSL status
simple-sftpd ssl status

# Renew certificate
simple-sftpd ssl renew --hostname DOMAIN
```

## Development

### Building from Source

```bash
# Clone and setup
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd

# Install dependencies
make install-dev

# Build options
make debug      # Debug build
make release    # Release build
make test       # Run tests
make clean      # Clean build artifacts

# Package creation
make package    # Create packages for current platform
```

### Project Structure

```
simple-sftpd/
├── include/simple-sftpd/     # Public headers
│   ├── core/                 # Server, connection, session tracking
│   ├── config/               # Configuration parsing (INI/JSON/YAML)
│   ├── user/                 # Users and user manager
│   ├── virtual_host/         # Virtual hosting (HOST command)
│   ├── security/             # SSL, PAM, rate limiting, IP access
│   └── utils/                # Logger, compression, file cache, metrics
├── src/simple-sftpd/         # Implementation (mirrors include/)
├── main/production.cpp       # Production CLI entry point
├── config/                   # Example configurations
├── tests/                    # Google Test unit and integration tests
├── docs/                     # User and developer documentation
├── automation/               # Ansible build VMs and CI helpers
├── deployment/               # Service files, Docker examples
├── packaging/                # Platform package assets
├── CMakeLists.txt
├── GNUmakefile / Makefile    # Make wrappers (FreeBSD uses gmake)
└── CHANGELOG.md
```

## 🐳 Docker Infrastructure

simple-sftpd includes comprehensive Docker support for development, testing, and production deployment:

### Docker Features

- **Multi-stage builds** for different Linux distributions (Ubuntu, CentOS, Alpine)
- **Multi-architecture support** (x86_64, ARM64, ARMv7)
- **Development environment** with debugging tools and live code mounting
- **Production-ready runtime** with minimal footprint and security hardening
- **Health checks** and monitoring capabilities
- **Volume mounts** for configuration, logs, and FTP data

### Quick Docker Commands

```bash
# Development environment
docker-compose --profile dev up -d

# Production deployment
docker-compose --profile runtime up -d

# Build for all platforms
./scripts/build-docker.sh -d all

# Deploy with custom configuration
./scripts/deploy-docker.sh -p runtime -c ./config -l ./logs -d ./data
```

### Docker Ports

- **21/tcp** - FTP control port
- **990/tcp** - FTPS control port (SSL/TLS)
- **1024-65535/tcp** - Passive mode data ports

For complete Docker documentation, see [Docker Deployment Guide](docs/shared/deployment/docker.md).

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

### Testing

```bash
# Run all tests
make test

# Run specific test suites
cd build && ctest -R "unit_tests"
cd build && ctest -R "integration_tests"

# Run with coverage (Linux only)
make coverage

# Run with memory checking (Linux only)
make memcheck
```

## Troubleshooting

### Common Issues

#### Server Won't Start

1. **Check configuration:**
```bash
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf
```

2. **Check permissions:**
```bash
sudo chown -R ftp:ftp /var/ftp
sudo chmod 755 /var/ftp
```

3. **Check ports:**
```bash
sudo netstat -tlnp | grep :21
```

#### SSL Connection Issues

1. **Verify certificate files:**
```bash
openssl x509 -in /etc/simple-sftpd/ssl/server.crt -text -noout
```

2. **Check certificate permissions:**
```bash
sudo chmod 600 /etc/simple-sftpd/ssl/server.key
sudo chown ftp:ftp /etc/simple-sftpd/ssl/server.key
```

#### Performance Issues

1. **Check system resources:**
```bash
top
iostat
netstat -i
```

2. **Adjust configuration:**
```ini
[connection]
max_connections = 50
thread_pool_size = 4

[transfer]
buffer_size = 16384
use_sendfile = true
```

### Log Files

- **Main log**: `/var/log/simple-sftpd/simple-sftpd.log`
- **Access log**: `/var/log/simple-sftpd/access.log`
- **Error log**: `/var/log/simple-sftpd/error.log`

### Debug Mode

Enable debug logging:

```ini
[logging]
log_level = "DEBUG"
log_commands = true
log_transfers = true

# Development settings
debug_mode = true
verbose_logging = true
trace_commands = true
```

## Security Considerations

### Production Deployment

1. **Use strong SSL certificates**
2. **Enable chroot for users**
3. **Drop privileges to non-root user**
4. **Implement rate limiting**
5. **Restrict allowed commands**
6. **Use firewall rules**
7. **Regular security updates**

### Network Security

```bash
# Firewall rules (iptables)
sudo iptables -A INPUT -p tcp --dport 21 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 1024:65535 -j ACCEPT

# Or with ufw (Ubuntu)
sudo ufw allow 21/tcp
sudo ufw allow 1024:65535/tcp
```

### User Security

1. **Strong password policies**
2. **Limited permissions**
3. **Path restrictions**
4. **Connection limits**
5. **Session timeouts**

## Performance Tuning

### System Tuning

```bash
# Increase file descriptor limits
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# Kernel parameters
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog = 65536" >> /etc/sysctl.conf
sysctl -p
```

### Application Tuning

```ini
[connection]
max_connections = 200
backlog = 100
keep_alive = true
tcp_nodelay = true

[transfer]
buffer_size = 32768
use_sendfile = true
use_mmap = true

# Performance settings
thread_pool_size = 16
enable_compression = true
cache_size = 100MB
```

## Monitoring and Metrics

### Built-in Monitoring

```bash
# Show server statistics
simple-sftpd status

# Show performance metrics
simple-sftpd metrics

# Show connection information
simple-sftpd connections
```

### External Monitoring

- **Prometheus**: Metrics endpoint at `/metrics`
- **Grafana**: Dashboard templates included
- **Log aggregation**: Structured logging support
- **Health checks**: HTTP health endpoint

## License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/blburns/simple-sftpd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/blburns/simple-sftpd/discussions)
- **Email**: SimpleDaemons

## Acknowledgments

- **OpenSSL**: SSL/TLS implementation
- **jsoncpp**: JSON parsing library
- **CMake**: Build system
- **FTP RFC 959**: Protocol specification

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for full release history.

| Version | Status | Highlights |
|---------|--------|------------|
| **v0.4.0** | Current | MODE Z compression, PORT/EPRT dispatch, protocol integration tests |
| **v0.3.0** | Released | Virtual hosting (HOST), persistent JSON users, groups, guest accounts, session/storage quotas |
| **v0.2.0** | Released | FTPS, PAM, chroot, active mode, resume/append/rename, connection pooling, sendfile/mmap, IPv6 |
| **v0.1.0** | Released | Core FTP server, passive mode, multi-format config, CLI, logging, rate limiting |

**Production line:** complete through v0.4.0. Enterprise / Datacenter remain planned.
