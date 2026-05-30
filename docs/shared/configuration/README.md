# Configuration Guide

This guide covers all aspects of configuring simple-sftpd, from basic server settings to advanced features like virtual hosting and SSL/TLS.

## 📁 Configuration Files

simple-sftpd uses a hierarchical configuration system with the following structure:

```
/etc/simple-sftpd/
├── simple-sftpd.conf              # Main server configuration
├── users/                   # User definitions
│   ├── admin.conf          # Admin user configuration
│   ├── user1.conf          # Regular user configuration
│   └── anonymous.conf      # Anonymous user configuration
├── virtual-hosts/           # Virtual host configurations
│   ├── example.com.conf    # Domain-specific configuration
│   └── ftp.example.com.conf # Subdomain configuration
└── ssl/                     # SSL certificate storage
    ├── server.crt          # Server certificate
    ├── server.key          # Private key
    └── ca-bundle.crt       # CA certificate bundle
```

## ⚙️ Main Configuration File

The main configuration file (`simple-sftpd.conf`) controls global server settings.

### Basic Server Configuration

```ini
[server]
# Server identification
server_name = "Simple-Secure FTP Daemon"
server_version = "0.3.0"

# Network settings
listen_address = "0.0.0.0"
listen_port = 21
max_connections = 100
connection_timeout = 300

# Security settings
enable_ssl = true
ssl_port = 990
require_ssl = false
chroot_enabled = true
privilege_dropping = true

# Performance settings
max_threads = 50
buffer_size = 8192
sendfile_enabled = true
```

### SSL/TLS Configuration

```ini
[ssl]
# Certificate settings
certificate_file = "/etc/simple-sftpd/ssl/server.crt"
private_key_file = "/etc/simple-sftpd/ssl/server.key"
ca_bundle_file = "/etc/simple-sftpd/ssl/ca-bundle.crt"

# SSL protocol settings
ssl_protocols = "TLSv1.2,TLSv1.3"
ssl_ciphers = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
ssl_prefer_server_ciphers = true

# Certificate validation
verify_client = false
client_ca_file = "/etc/simple-sftpd/ssl/client-ca.crt"
```

### Logging Configuration

```ini
[logging]
# Log file
log_file = "/var/log/simple-sftpd/simple-sftpd.log"

# Log level: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
log_level = "INFO"

# Log format: STANDARD, JSON, or EXTENDED
# STANDARD: [timestamp] [LEVEL] message
# JSON: {"timestamp":"...","level":"...","message":"..."}
# EXTENDED: [timestamp] [LEVEL] [PID:...] message
log_format = "STANDARD"

# Output destinations
log_to_console = true
log_to_file = true
```

**Log Format Examples:**

**STANDARD Format:**
```
[2024-11-25 16:30:45.123] [INFO] FTP Server started on 0.0.0.0:21
```

**JSON Format:**
```json
{"timestamp":"2024-11-25 16:30:45.123","level":"INFO","message":"FTP Server started on 0.0.0.0:21"}
```

**EXTENDED Format:**
```
[2024-11-25 16:30:45.123] [INFO] [PID:12345] FTP Server started on 0.0.0.0:21
```

The JSON format is ideal for log aggregation systems like ELK Stack, Splunk, or cloud logging services, as it provides structured, machine-readable log entries.

### Rate Limiting

```ini
[rate_limit]
# Global rate limiting
enable_rate_limiting = true
max_connections_per_minute = 60
max_connections_per_hour = 1000

# Per-IP rate limiting
per_ip_limiting = true
max_connections_per_ip_per_minute = 10
max_connections_per_ip_per_hour = 100

# Per-user rate limiting
per_user_limiting = true
max_connections_per_user_per_minute = 5
max_connections_per_user_per_hour = 50

# Transfer rate limiting
max_transfer_rate = "10MB/s"
max_transfer_rate_per_user = "5MB/s"
```

## 👥 User Configuration

User configurations are stored in separate files under `/etc/simple-sftpd/users/`.

### Admin User Configuration

```ini
[user:admin]
# Basic user info
username = "admin"
password_hash = "$2y$10$..."
full_name = "Administrator"
email = "admin@example.com"
description = "System administrator"

# Home directory
home_directory = "/var/ftp/admin"
chroot_directory = "/var/ftp"

# Permissions
permissions = "READ,WRITE,DELETE,UPLOAD,DOWNLOAD,ADMIN"
can_create_directories = true
can_delete_files = true
can_rename_files = true

# Connection limits
max_connections = 10
max_concurrent_transfers = 5

# Transfer limits
max_file_size = "1GB"
max_transfer_rate = "50MB/s"
max_daily_transfer = "10GB"

# Session settings
session_timeout = 3600
idle_timeout = 1800
```

### Regular User Configuration

```ini
[user:user1]
# Basic user info
username = "user1"
password_hash = "$2y$10$..."
full_name = "Regular User"
email = "user1@example.com"

# Home directory
home_directory = "/var/ftp/user1"
chroot_directory = "/var/ftp/user1"

# Permissions
permissions = "READ,WRITE,UPLOAD,DOWNLOAD"
can_create_directories = true
can_delete_files = false
can_rename_files = false

# Connection limits
max_connections = 3
max_concurrent_transfers = 2

# Transfer limits
max_file_size = "100MB"
max_transfer_rate = "10MB/s"
max_daily_transfer = "1GB"

# Session settings
session_timeout = 1800
idle_timeout = 900
```

### Anonymous User Configuration

```ini
[user:anonymous]
# Basic user info
username = "anonymous"
password = ""
full_name = "Anonymous User"
email = "anonymous@example.com"

# Home directory
home_directory = "/var/ftp/public"
chroot_directory = "/var/ftp/public"

# Permissions
permissions = "READ,DOWNLOAD"
can_create_directories = false
can_delete_files = false
can_rename_files = false

# Connection limits
max_connections = 5
max_concurrent_transfers = 1

# Transfer limits
max_file_size = "50MB"
max_transfer_rate = "5MB/s"
max_daily_transfer = "500MB"

# Session settings
session_timeout = 900
idle_timeout = 300
```

## 🌐 Virtual Host Configuration

Virtual hosts allow you to serve different FTP sites for different domains.

### Basic Virtual Host

```ini
[virtual_host:example.com]
# Domain settings
hostname = "example.com"
ip_address = "192.168.1.100"

# FTP settings
ftp_port = 21
ssl_port = 990
enable_ssl = true

# Document root
document_root = "/var/ftp/example.com"
chroot_directory = "/var/ftp"

# SSL certificate
ssl_certificate = "/etc/simple-sftpd/ssl/example.com.crt"
ssl_private_key = "/etc/simple-sftpd/ssl/example.com.key"

# User access
allowed_users = "admin,user1,anonymous"
default_user = "anonymous"

# Custom settings
max_connections = 50
connection_timeout = 600
```

### Advanced Virtual Host

```ini
[virtual_host:ftp.example.com]
# Domain settings
hostname = "ftp.example.com"
ip_address = "192.168.1.101"

# FTP settings
ftp_port = 2121
ssl_port = 9990
enable_ssl = true
require_ssl = true

# Document root
document_root = "/var/ftp/ftp.example.com"
chroot_directory = "/var/ftp/ftp.example.com"

# SSL certificate
ssl_certificate = "/etc/simple-sftpd/ssl/ftp.example.com.crt"
ssl_private_key = "/etc/simple-sftpd/ssl/ftp.example.com.key"

# User access
allowed_users = "admin,user2"
default_user = "user2"

# Custom settings
max_connections = 25
connection_timeout = 300
max_file_size = "500MB"
max_transfer_rate = "25MB/s"

# Custom welcome message
welcome_message = "Welcome to ftp.example.com - Secure File Transfer"
```

## 🔒 Security Configuration

### Access Control

```ini
[security]
# IP restrictions
allowed_networks = "192.168.1.0/24,10.0.0.0/8"
blocked_networks = "192.168.1.100,10.0.0.100"
enable_geo_blocking = false
allowed_countries = "US,CA,GB,DE"

# User restrictions
max_failed_logins = 3
account_lockout_duration = 1800
password_policy = "strong"
min_password_length = 8
require_special_chars = true

# Session security
enable_session_tracking = true
max_sessions_per_user = 5
session_timeout = 3600
idle_timeout = 1800

# File security
scan_uploads = true
max_filename_length = 255
forbidden_extensions = "exe,com,bat,sh"
```

### Chroot Configuration

```ini
[chroot]
# Global chroot settings
enable_chroot = true
chroot_base = "/var/ftp"

# Per-user chroot
user_chroot_enabled = true
user_chroot_base = "/var/ftp/users"

# Chroot exceptions
chroot_exceptions = "admin"
admin_chroot_base = "/var/ftp"

# Chroot security
chroot_allow_symlinks = false
chroot_allow_hardlinks = false
chroot_allow_devices = false
```

## 📊 Monitoring and Statistics

### Performance Monitoring

```ini
[monitoring]
# Enable monitoring
enable_monitoring = true
monitoring_port = 8080
monitoring_interface = "127.0.0.1"

# Metrics collection
collect_connection_stats = true
collect_transfer_stats = true
collect_user_stats = true
collect_system_stats = true

# Statistics retention
stats_retention_days = 30
stats_cleanup_interval = 86400

# Performance thresholds
max_cpu_usage = 80
max_memory_usage = 80
max_disk_usage = 90
```

### Logging and Auditing

```ini
[auditing]
# Audit logging
enable_audit_log = true
audit_log_file = "/var/log/simple-sftpd/audit.log"
audit_log_level = "info"

# Audit events
audit_user_login = true
audit_user_logout = true
audit_file_upload = true
audit_file_download = true
audit_file_delete = true
audit_directory_create = true
audit_directory_delete = true

# Audit retention
audit_log_retention_days = 90
audit_log_rotation = true
```

## 🔧 Advanced Configuration

### Custom Commands

```ini
[commands]
# Enable/disable commands
enable_help = true
enable_site = true
enable_syst = true
enable_stat = true
enable_noop = true

# Custom commands
custom_commands = "version,status,info"
version_command = "simple-sftpd version 0.1.0"
status_command = "Server is running normally"
info_command = "Simple-Secure FTP Daemon"
```

### File Transfer Optimization

```ini
[transfer]
# Buffer settings
read_buffer_size = 8192
write_buffer_size = 8192
transfer_buffer_size = 65536

# Optimization
enable_sendfile = true
enable_mmap = true
enable_zero_copy = true

# Compression
enable_compression = false
compression_level = 6
compression_types = "gzip,bzip2"

# Checksums
verify_checksums = true
checksum_algorithms = "md5,sha1,sha256"
```

## 📝 Configuration Examples

### Minimal Configuration

```ini
[server]
listen_address = "0.0.0.0"
listen_port = 21
enable_ssl = false

[logging]
log_level = "info"
log_file = "/var/log/simple-sftpd/simple-sftpd.log"

[user:anonymous]
username = "anonymous"
password = ""
home_directory = "/var/ftp/public"
permissions = "READ,DOWNLOAD"
```

### Production Configuration

```ini
[server]
server_name = "Production FTP Server"
listen_address = "0.0.0.0"
listen_port = 21
max_connections = 200
connection_timeout = 600

[ssl]
enable_ssl = true
certificate_file = "/etc/simple-sftpd/ssl/server.crt"
private_key_file = "/etc/simple-sftpd/ssl/server.key"
require_ssl = true

[security]
enable_chroot = true
chroot_base = "/var/ftp"
max_failed_logins = 3
account_lockout_duration = 1800

[monitoring]
enable_monitoring = true
monitoring_port = 8080
collect_all_stats = true

[rate_limit]
enable_rate_limiting = true
max_connections_per_minute = 100
max_transfer_rate = "50MB/s"
```

### High-Security Configuration

```ini
[server]
server_name = "High-Security FTP Server"
listen_address = "192.168.1.100"
listen_port = 21
max_connections = 50

[ssl]
enable_ssl = true
require_ssl = true
certificate_file = "/etc/simple-sftpd/ssl/server.crt"
private_key_file = "/etc/simple-sftpd/ssl/server.key"
ssl_protocols = "TLSv1.3"
ssl_ciphers = "ECDHE-RSA-AES256-GCM-SHA384"

[security]
enable_chroot = true
chroot_base = "/var/ftp"
privilege_dropping = true
allowed_networks = "192.168.1.0/24"
max_failed_logins = 2
account_lockout_duration = 3600
password_policy = "strong"
min_password_length = 12

[auditing]
enable_audit_log = true
audit_all_events = true
audit_log_retention_days = 365
```

## 🔍 Configuration Validation

### Test Configuration

```bash
# Test configuration file syntax
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf

# Test configuration with specific user
simple-sftpd --test-config --config /etc/simple-sftpd/simple-sftpd.conf --user admin

# Validate SSL configuration
simple-sftpd --test-ssl --config /etc/simple-sftpd/simple-sftpd.conf
```

### Configuration Check

```bash
# Check configuration file permissions
ls -la /etc/simple-sftpd/simple-sftpd.conf

# Verify SSL certificates
openssl x509 -in /etc/simple-sftpd/ssl/server.crt -text -noout

# Check user configurations
simple-sftpd-ctl validate-users --config /etc/simple-sftpd/simple-sftpd.conf
```

## 📚 Next Steps

After configuring your server:

1. **Test the configuration**: Use `simple-sftpd --test-config`
2. **Start the server**: Use `simple-sftpd --config /etc/simple-sftpd/simple-sftpd.conf`
3. **Set up users**: See [User Management](../user-guide/user-management.md)
4. **Configure SSL**: See [SSL Configuration](ssl.md)
5. **Set up monitoring**: See [Monitoring Guide](../user-guide/monitoring.md)

---

**Need help?** Check our [Configuration Examples](../examples/README.md) or open an [issue on GitHub](https://github.com/blburns/simple-sftpd/issues).
