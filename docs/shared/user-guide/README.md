# User Guide

This comprehensive guide covers all aspects of using simple-sftpd, from basic operations to advanced features and troubleshooting.

## 🚀 Quick Start

### Starting the Server

```bash
# Start with default configuration
sudo simple-sftpd start

# Start with custom configuration
sudo simple-sftpd start --config /etc/simple-sftpd/simple-sftpd.conf

# Start in foreground mode (for testing)
sudo simple-sftpd --config /etc/simple-sftpd/simple-sftpd.conf --foreground

# Start as daemon
sudo simple-sftpd --daemon start --config /etc/simple-sftpd/simple-sftpd.conf
```

### Basic Commands

```bash
# Check server status
simple-sftpd status

# Stop the server
sudo simple-sftpd stop

# Restart the server
sudo simple-sftpd restart

# Reload configuration (sends SIGHUP, full reload requires restart in v0.1.0)
sudo simple-sftpd reload

# Test configuration file
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf

# Check server version
simple-sftpd --version
```

## 👥 User Management

### Creating Users

```bash
# Create a new user
sudo simple-sftpd user add \
  --username john \
  --password securepass123 \
  --home /var/ftp/john \
  --permissions READ,WRITE,UPLOAD,DOWNLOAD

# Create user with specific limits
sudo simple-sftpd user add \
  --username limited \
  --password pass123 \
  --home /var/ftp/limited \
  --permissions READ,DOWNLOAD \
  --max-connections 2 \
  --max-file-size 100MB \
  --transfer-rate 5MB/s

# Create admin user
sudo simple-sftpd user add \
  --username admin \
  --password adminpass \
  --home /var/ftp/admin \
  --permissions READ,WRITE,DELETE,UPLOAD,DOWNLOAD,ADMIN \
  --max-connections 10
```

### Managing Users

```bash
# List all users
simple-sftpd user list

# Get user details
simple-sftpd user info --username john

# Modify user permissions
sudo simple-sftpd user modify \
  --username john \
  --permissions READ,WRITE,UPLOAD,DOWNLOAD,DELETE

# Change user password
sudo simple-sftpd user password \
  --username john \
  --password newpassword123

# Disable user
sudo simple-sftpd user disable --username john

# Enable user
sudo simple-sftpd user enable --username john

# Remove user
sudo simple-sftpd user remove --username john
```

### User Types and Permissions

#### Permission Levels

| Permission | Description | Commands |
|------------|-------------|----------|
| `READ` | View files and directories | `LIST`, `PWD`, `CWD` |
| `WRITE` | Modify file attributes | `CHMOD`, `CHOWN` |
| `UPLOAD` | Upload files | `STOR`, `APPE`, `STOU` |
| `DOWNLOAD` | Download files | `RETR` |
| `DELETE` | Delete files and directories | `DELE`, `RMD` |
| `ADMIN` | Administrative operations | All commands |

#### User Types

**Regular Users**
- Full access to their home directory
- Configurable permissions and limits
- Session tracking and statistics

**Anonymous Users**
- Limited access to public directories
- No authentication required
- Restricted permissions and limits

**Guest Users**
- Temporary access with restrictions
- Limited session duration
- Minimal permissions

**Admin Users**
- Full system access
- User management capabilities
- Configuration modification rights

## 🌐 Virtual Host Management

### Creating Virtual Hosts

```bash
# Create basic virtual host
sudo simple-sftpd virtual add \
  --hostname ftp.example.com \
  --root /var/ftp/example.com \
  --ssl-cert /etc/simple-sftpd/ssl/example.com.crt \
  --ssl-key /etc/simple-sftpd/ssl/example.com.key

# Create virtual host with custom settings
sudo simple-sftpd virtual add \
  --hostname secure.example.com \
  --root /var/ftp/secure \
  --port 2121 \
  --ssl-port 9990 \
  --require-ssl \
  --max-connections 50 \
  --allowed-users admin,user1
```

### Managing Virtual Hosts

```bash
# List all virtual hosts
simple-sftpd virtual list

# Get virtual host details
simple-sftpd virtual info --hostname ftp.example.com

# Enable virtual host
sudo simple-sftpd virtual enable --hostname ftp.example.com

# Disable virtual host
sudo simple-sftpd virtual disable --hostname ftp.example.com

# Remove virtual host
sudo simple-sftpd virtual remove --hostname ftp.example.com
```

### Virtual Host Configuration

```bash
# Update virtual host settings
sudo simple-sftpd virtual modify \
  --hostname ftp.example.com \
  --max-connections 100 \
  --ssl-cert /etc/simple-sftpd/ssl/new-cert.crt \
  --ssl-key /etc/simple-sftpd/ssl/new-key.key

# Set default virtual host
sudo simple-sftpd virtual set-default --hostname ftp.example.com

# Test virtual host configuration
simple-sftpd virtual test --hostname ftp.example.com
```

## 🔒 SSL Certificate Management

### Generating Certificates

```bash
# Generate self-signed certificate
sudo simple-sftpd ssl generate \
  --hostname ftp.example.com \
  --country US \
  --state California \
  --city "San Francisco" \
  --organization "Example Corp" \
  --email admin@example.com \
  --days 365

# Generate certificate with custom settings
sudo simple-sftpd ssl generate \
  --hostname ftp.example.com \
  --key-size 4096 \
  --signature-algorithm sha256 \
  --days 730
```

### Installing Certificates

```bash
# Install existing certificate
sudo simple-sftpd ssl install \
  --hostname ftp.example.com \
  --cert /path/to/certificate.crt \
  --key /path/to/private.key

# Install with CA certificate
sudo simple-sftpd ssl install \
  --hostname ftp.example.com \
  --cert /path/to/certificate.crt \
  --key /path/to/private.key \
  --ca /path/to/ca.crt

# Install Let's Encrypt certificate
sudo simple-sftpd ssl install \
  --hostname ftp.example.com \
  --cert /etc/letsencrypt/live/ftp.example.com/fullchain.pem \
  --key /etc/letsencrypt/live/ftp.example.com/privkey.pem
```

### Managing Certificates

```bash
# List all certificates
simple-sftpd ssl list

# Check certificate status
simple-sftpd ssl status --hostname ftp.example.com

# Renew certificate
sudo simple-sftpd ssl renew --hostname ftp.example.com

# Remove certificate
sudo simple-sftpd ssl remove --hostname ftp.example.com

# Validate certificate
simple-sftpd ssl validate --hostname ftp.example.com
```

## 📊 Monitoring and Statistics

### Server Status

```bash
# Get server status
simple-sftpd status

# Get detailed status
simple-sftpd status --detailed

# Get status in JSON format
simple-sftpd status --json

# Get status for specific virtual host
simple-sftpd status --hostname ftp.example.com
```

### Connection Information

```bash
# List active connections
simple-sftpd connections

# Get connection details
simple-sftpd connections --detailed

# Filter connections by user
simple-sftpd connections --user john

# Filter connections by IP
simple-sftpd connections --ip 192.168.1.100
```

### Transfer Statistics

```bash
# Get transfer statistics
simple-sftpd transfers

# Get user transfer stats
simple-sftpd transfers --user john

# Get virtual host transfer stats
simple-sftpd transfers --hostname ftp.example.com

# Get transfer stats for time period
simple-sftpd transfers --since "2024-01-01" --until "2024-01-31"
```

### Performance Metrics

```bash
# Get performance metrics
simple-sftpd metrics

# Get system resource usage
simple-sftpd metrics --system

# Get network statistics
simple-sftpd metrics --network

# Get disk usage statistics
simple-sftpd metrics --disk
```

## 📝 Logging and Troubleshooting

### Viewing Logs

```bash
# View main server log
sudo tail -f /var/log/simple-sftpd/simple-sftpd.log

# View access log
sudo tail -f /var/log/simple-sftpd/access.log

# View error log
sudo tail -f /var/log/simple-sftpd/error.log

# View audit log
sudo tail -f /var/log/simple-sftpd/audit.log

# Search logs for specific user
sudo grep "john" /var/log/simple-sftpd/simple-sftpd.log

# Search logs for specific IP
sudo grep "192.168.1.100" /var/log/simple-sftpd/access.log
```

### Log Analysis

```bash
# Get log summary
simple-sftpd logs summary

# Get error count
simple-sftpd logs errors --count

# Get access patterns
simple-sftpd logs access --pattern

# Get user activity
simple-sftpd logs user --username john

# Export logs
simple-sftpd logs export --format csv --output /tmp/logs.csv
```

### Common Issues and Solutions

#### Connection Problems

```bash
# Check if server is running
simple-sftpd status

# Check if port is open
sudo netstat -tlnp | grep :21

# Check firewall rules
sudo ufw status
sudo firewall-cmd --list-all

# Test local connection
ftp localhost
```

#### Authentication Issues

```bash
# Check user configuration
simple-sftpd user info --username john

# Verify password
sudo simple-sftpd user password --username john --password newpass

# Check user permissions
simple-sftpd user permissions --username john

# Test user login
simple-sftpd user test --username john
```

#### SSL/TLS Issues

```bash
# Check certificate validity
simple-sftpd ssl status --hostname ftp.example.com

# Validate certificate
simple-sftpd ssl validate --hostname ftp.example.com

# Check SSL configuration
simple-sftpd ssl config --hostname ftp.example.com

# Test SSL connection
openssl s_client -connect ftp.example.com:990
```

## 🔧 Advanced Features

### Custom Commands

```bash
# Enable custom commands
simple-sftpd commands enable --command version

# Disable custom commands
simple-sftpd commands disable --command help

# List available commands
simple-sftpd commands list

# Test custom command
simple-sftpd commands test --command version
```

### File Transfer Optimization

```bash
# Enable sendfile optimization
simple-sftpd transfer optimize --sendfile

# Enable memory mapping
simple-sftpd transfer optimize --mmap

# Set buffer sizes
simple-sftpd transfer buffer --read 16384 --write 16384

# Enable compression
simple-sftpd transfer compression --enable --level 6
```

### Rate Limiting

```bash
# Set global rate limits
sudo simple-sftpd rate-limit set \
  --connections-per-minute 100 \
  --transfers-per-minute 50 \
  --bytes-per-minute 100MB

# Set per-user rate limits
sudo simple-sftpd rate-limit user \
  --username john \
  --connections-per-minute 10 \
  --transfer-rate 5MB/s

# Set per-IP rate limits
sudo simple-sftpd rate-limit ip \
  --ip 192.168.1.100 \
  --connections-per-minute 5 \
  --transfer-rate 2MB/s
```

## 📱 Web Interface

### Accessing the Web Interface

```bash
# Enable web interface
sudo simple-sftpd web enable --port 8080

# Access web interface
# Open browser to: http://localhost:8080

# Set web interface authentication
sudo simple-sftpd web auth \
  --username admin \
  --password adminpass

# Configure web interface
sudo simple-sftpd web config \
  --theme dark \
  --language en \
  --timezone UTC
```

### Web Interface Features

- **Server Status**: Real-time server information
- **User Management**: Add, modify, and remove users
- **Virtual Host Management**: Configure virtual hosts
- **SSL Certificate Management**: Manage SSL certificates
- **Monitoring**: View statistics and performance metrics
- **Logs**: Browse and search log files
- **File Browser**: Browse FTP directories

## 🔄 Backup and Recovery

### Configuration Backup

```bash
# Backup all configurations
sudo simple-sftpd backup create --output /backup/simple-sftpd-$(date +%Y%m%d).tar.gz

# Backup specific configuration
sudo simple-sftpd backup config --output /backup/config-$(date +%Y%m%d).tar.gz

# Backup user configurations
sudo simple-sftpd backup users --output /backup/users-$(date +%Y%m%d).tar.gz

# Backup SSL certificates
sudo simple-sftpd backup ssl --output /backup/ssl-$(date +%Y%m%d).tar.gz
```

### Configuration Restoration

```bash
# Restore from backup
sudo simple-sftpd backup restore --file /backup/simple-sftpd-20240115.tar.gz

# Restore specific configuration
sudo simple-sftpd backup restore-config --file /backup/config-20240115.tar.gz

# Restore users
sudo simple-sftpd backup restore-users --file /backup/users-20240115.tar.gz

# Restore SSL certificates
sudo simple-sftpd backup restore-ssl --file /backup/ssl-20240115.tar.gz
```

## 📚 Command Reference

### Server Management Commands

| Command | Description | Options |
|---------|-------------|---------|
| `simple-sftpd start` | Start the server | `--config`, `--daemon`, `--foreground` |
| `simple-sftpd stop` | Stop the server | `--force`, `--timeout` |
| `simple-sftpd restart` | Restart the server | `--config`, `--daemon` |
| `simple-sftpd reload` | Reload configuration | `--config` |
| `simple-sftpd status` | Show server status | `--detailed`, `--json` |

### User Management Commands

| Command | Description | Options |
|---------|-------------|---------|
| `simple-sftpd user add` | Add new user | `--username`, `--password`, `--home`, `--permissions` |
| `simple-sftpd user modify` | Modify user | `--username`, `--permissions`, `--limits` |
| `simple-sftpd user remove` | Remove user | `--username`, `--force` |
| `simple-sftpd user list` | List users | `--detailed`, `--json` |
| `simple-sftpd user info` | Show user info | `--username` |

### Virtual Host Commands

| Command | Description | Options |
|---------|-------------|---------|
| `simple-sftpd virtual add` | Add virtual host | `--hostname`, `--root`, `--ssl-cert`, `--ssl-key` |
| `simple-sftpd virtual modify` | Modify virtual host | `--hostname`, `--settings` |
| `simple-sftpd virtual remove` | Remove virtual host | `--hostname`, `--force` |
| `simple-sftpd virtual list` | List virtual hosts | `--detailed`, `--json` |
| `simple-sftpd virtual info` | Show virtual host info | `--hostname` |

### SSL Commands

| Command | Description | Options |
|---------|-------------|---------|
| `simple-sftpd ssl generate` | Generate certificate | `--hostname`, `--country`, `--state`, `--city` |
| `simple-sftpd ssl install` | Install certificate | `--hostname`, `--cert`, `--key`, `--ca` |
| `simple-sftpd ssl renew` | Renew certificate | `--hostname` |
| `simple-sftpd ssl remove` | Remove certificate | `--hostname` |
| `simple-sftpd ssl status` | Show certificate status | `--hostname` |

## 🚨 Troubleshooting Guide

### Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `Connection refused` | Server not running | Start server with `simple-sftpd start` |
| `Authentication failed` | Invalid credentials | Check username/password |
| `Permission denied` | Insufficient permissions | Check user permissions |
| `SSL handshake failed` | Certificate issues | Validate SSL configuration |
| `Port already in use` | Port conflict | Change port or stop conflicting service |

### Diagnostic Commands

```bash
# Test server configuration
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf

# Test SSL configuration
simple-sftpd --test-ssl --config /etc/simple-sftpd/simple-sftpd.conf

# Validate user configurations
simple-sftpd-ctl validate-users --config /etc/simple-sftpd/simple-sftpd.conf

# Check file permissions
simple-sftpd-ctl check-permissions --config /etc/simple-sftpd/simple-sftpd.conf

# Test network connectivity
simple-sftpd-ctl test-network --host localhost --port 21
```

### Performance Tuning

```bash
# Monitor performance
simple-sftpd metrics --real-time

# Analyze bottlenecks
simple-sftpd analyze --performance

# Optimize settings
simple-sftpd optimize --auto

# Generate performance report
simple-sftpd report --performance --output /tmp/performance.pdf
```

## 📚 Next Steps

After mastering the basic usage:

1. **Advanced Configuration**: See [Configuration Guide](../configuration/README.md)
2. **Security Hardening**: See [Security Guide](security.md)
3. **Performance Tuning**: See [Performance Guide](performance.md)
4. **Integration**: See [Integration Guide](integration.md)
5. **Development**: See [Development Guide](../development/README.md)

---

**Need help?** Check [Production Troubleshooting](../../production/troubleshooting.md) or open an [issue on GitHub](https://github.com/blburns/simple-sftpd/issues).
