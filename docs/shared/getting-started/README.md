# Installation Guide

**Production version:** v0.4.0

This guide covers installing simple-sftpd on supported platforms: Linux, macOS, FreeBSD, and Windows.

## 📋 Prerequisites

### System Requirements

- **Operating System**: Linux (kernel 3.10+), macOS 12.0+, or Windows 10/11
- **Architecture**: x86_64, ARM64 (Linux/macOS), or ARM64 (Windows)
- **Memory**: Minimum 512MB RAM, recommended 2GB+
- **Storage**: Minimum 100MB for installation, additional space for logs and data
- **Network**: Network interface for FTP connections

### Software Dependencies

#### Required Dependencies
- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake 3.16+**: Build system
- **OpenSSL 1.1.1+**: SSL/TLS support
- **jsoncpp**: JSON configuration parsing
- **pthread**: Threading support (Linux/macOS)

#### Optional Dependencies
- **PAM**: Pluggable Authentication Modules (Linux)
- **LDAP**: Lightweight Directory Access Protocol
- **SQLite**: Local user database
- **MySQL/PostgreSQL**: External user database

## 🚀 Quick Installation

### From Pre-built Packages (Recommended)

#### Ubuntu/Debian
```bash
# Add repository (if available)
sudo apt update
sudo apt install simple-sftpd

# Or install from .deb package
wget https://github.com/blburns/simple-sftpd/releases/latest/download/simple-sftpd_amd64.deb
sudo dpkg -i simple-sftpd_amd64.deb
sudo apt-get install -f  # Fix any dependency issues
```

#### CentOS/RHEL/Fedora
```bash
# Install from .rpm package
sudo yum install https://github.com/blburns/simple-sftpd/releases/latest/download/simple-sftpd_x86_64.rpm

# Or for newer systems
sudo dnf install https://github.com/blburns/simple-sftpd/releases/latest/download/simple-sftpd_x86_64.rpm
```

#### macOS
```bash
# Using Homebrew
brew install simple-sftpd

# Or install from .pkg package
curl -LO https://github.com/blburns/simple-sftpd/releases/latest/download/simple-sftpd_macos.pkg
sudo installer -pkg simple-sftpd_macos.pkg -target /
```

#### Windows
```powershell
# Using Chocolatey
choco install simple-sftpd

# Or download and run .msi installer
# Download from: https://github.com/blburns/simple-sftpd/releases/latest/download/simple-sftpd_windows.msi
```

### From Source Code

#### 1. Clone the Repository
```bash
git clone https://github.com/blburns/simple-sftpd.git
cd simple-sftpd
```

#### 2. Install Development Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev libjsoncpp-dev git
```

**CentOS/RHEL/Fedora:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake openssl-devel jsoncpp-devel git
```

**macOS:**
```bash
brew install cmake openssl jsoncpp
```

**Windows:**
```powershell
# Install Visual Studio Build Tools or Visual Studio Community
# Install CMake from https://cmake.org/download/
# Install vcpkg for dependencies
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg install openssl jsoncpp
```

#### 3. Build and Install
```bash
# Build the project
make build

# Install system-wide
sudo make install

# Or install to custom directory
make install DESTDIR=/opt/simple-sftpd
```

## 🔧 Platform-Specific Installation

### Linux Installation

#### Systemd Service Setup
```bash
# Enable and start the service
sudo systemctl enable simple-sftpd
sudo systemctl start simple-sftpd

# Check status
sudo systemctl status simple-sftpd

# View logs
sudo journalctl -u simple-sftpd -f
```

#### Firewall Configuration
```bash
# UFW (Ubuntu)
sudo ufw allow 21/tcp
sudo ufw allow 30000:31000/tcp  # Passive mode ports

# firewalld (CentOS/RHEL)
sudo firewall-cmd --permanent --add-service=ftp
sudo firewall-cmd --permanent --add-port=30000-31000/tcp
sudo firewall-cmd --reload

# iptables
sudo iptables -A INPUT -p tcp --dport 21 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 30000:31000 -j ACCEPT
```

#### SELinux Configuration (RHEL/CentOS)
```bash
# Allow FTP through SELinux
sudo setsebool -P ftpd_full_access 1
sudo setsebool -P ftpd_use_passive_mode 1
```

### macOS Installation

#### LaunchDaemon Setup
```bash
# Create LaunchDaemon plist
sudo cp /usr/local/share/simple-sftpd/org.simple-sftpd.plist /Library/LaunchDaemons/

# Load and start the service
sudo launchctl load /Library/LaunchDaemons/org.simple-sftpd.plist
sudo launchctl start org.simple-sftpd

# Check status
sudo launchctl list | grep simple-sftpd
```

#### Firewall Configuration
```bash
# Allow incoming FTP connections
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/local/bin/simple-sftpd
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /usr/local/bin/simple-sftpd
```

### Windows Installation

#### Windows Service Setup
```powershell
# Install as Windows Service
sc create simple-sftpd binPath= "C:\Program Files\simple-sftpd\bin\simple-sftpd.exe --service"
sc description simple-sftpd "Simple-Secure FTP Daemon"
sc start simple-sftpd

# Or use the installer which sets this up automatically
```

#### Windows Firewall
```powershell
# Allow FTP through Windows Firewall
netsh advfirewall firewall add rule name="simple-sftpd FTP" dir=in action=allow protocol=TCP localport=21
netsh advfirewall firewall add rule name="simple-sftpd Passive" dir=in action=allow protocol=TCP localport=30000-31000
```

## 📁 Installation Directory Structure

After installation, the following directory structure is created:

```
/etc/simple-sftpd/           # Configuration files
├── simple-sftpd.conf        # Main configuration
├── users/             # User definitions
├── virtual-hosts/     # Virtual host configurations
└── ssl/               # SSL certificates

/usr/local/bin/        # Executables
├── simple-sftpd             # Main server binary
├── simple-sftpd-ctl         # Control utility
└── simple-sftpd-user        # User management utility

/usr/local/share/simple-sftpd/ # Shared files
├── examples/          # Example configurations
├── scripts/           # Utility scripts
└── templates/         # Configuration templates

/var/log/simple-sftpd/       # Log files
├── simple-sftpd.log         # Main server log
├── access.log         # Access log
└── error.log          # Error log

/var/ftp/              # Default FTP root directory
└── public/            # Public files
```

## ⚙️ Post-Installation Configuration

### 1. Initial Configuration
```bash
# Copy example configuration
sudo cp /etc/simple-sftpd/simple-sftpd.conf.example /etc/simple-sftpd/simple-sftpd.conf

# Edit configuration
sudo nano /etc/simple-sftpd/simple-sftpd.conf
```

### 2. Create FTP User
```bash
# Create FTP user account
sudo simple-sftpd user add --username ftpuser --password securepass --home /var/ftp/ftpuser

# Or use the management script
sudo tools/manage-users.sh add ftpuser securepass /var/ftp/ftpuser
```

### 3. Set Up SSL (Optional but Recommended)
```bash
# Generate self-signed certificate
sudo simple-sftpd ssl generate --hostname localhost

# Or use Let's Encrypt
sudo simple-sftpd ssl install --hostname yourdomain.com --cert /etc/letsencrypt/live/yourdomain.com/fullchain.pem --key /etc/letsencrypt/live/yourdomain.com/privkey.pem
```

### 4. Test Configuration
```bash
# Test configuration file
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf

# Start server in foreground for testing
simple-sftpd --config /etc/simple-sftpd/simple-sftpd.conf --foreground
```

## 🔍 Verification

### Check Installation
```bash
# Verify binary installation
which simple-sftpd
simple-sftpd --version

# Check service status
sudo systemctl status simple-sftpd  # Linux
sudo launchctl list | grep simple-sftpd  # macOS
sc query simple-sftpd  # Windows
```

### Test FTP Connection
```bash
# Test local connection
ftp localhost

# Or use command line client
simple-sftpd-ctl test-connection localhost 21
```

## 🚨 Troubleshooting

### Common Installation Issues

#### Missing Dependencies
```bash
# Check for missing libraries
ldd /usr/local/bin/simple-sftpd

# Install missing packages
sudo apt install libssl1.1 libjsoncpp1  # Ubuntu/Debian
sudo yum install openssl jsoncpp        # CentOS/RHEL
```

#### Permission Issues
```bash
# Fix ownership
sudo chown -R ftp:ftp /var/ftp
sudo chmod -R 755 /var/ftp

# Fix configuration permissions
sudo chown root:root /etc/simple-sftpd/simple-sftpd.conf
sudo chmod 600 /etc/simple-sftpd/simple-sftpd.conf
```

#### Port Already in Use
```bash
# Check what's using port 21
sudo netstat -tlnp | grep :21
sudo lsof -i :21

# Kill conflicting process or change port in configuration
```

### Getting Help

- **Check logs**: `/var/log/simple-sftpd/simple-sftpd.log`
- **Configuration errors**: `simple-sftpd --test-config`
- **Service issues**: Check system service status
- **Community support**: [GitHub Issues](https://github.com/blburns/simple-sftpd/issues)

## 🔄 Upgrading

### Package Manager Upgrade
```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade simple-sftpd

# CentOS/RHEL
sudo yum update simple-sftpd

# macOS
brew upgrade simple-sftpd
```

### Source Code Upgrade
```bash
# Pull latest changes
git pull origin main

# Rebuild and reinstall
make clean
make build
sudo make install

# Restart service
sudo systemctl restart simple-sftpd  # Linux
sudo launchctl restart org.simple-sftpd  # macOS
```

## 📚 Next Steps

After successful installation:

1. **Configure the server**: See [Configuration Guide](../configuration/README.md)
2. **Set up users**: See [User Management](../user-guide/user-management.md)
3. **Enable SSL**: See [SSL Configuration](../configuration/ssl.md)
4. **Configure virtual hosts**: See [Virtual Hosting](../configuration/virtual-hosts.md)

---

**Need help?** See [Production Troubleshooting](../production/troubleshooting.md) or open an [issue on GitHub](https://github.com/blburns/simple-sftpd/issues).
