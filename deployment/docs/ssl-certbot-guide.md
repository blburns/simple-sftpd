# SSL Certificate Management with Certbot

This guide explains how to use Certbot to obtain and manage SSL certificates for simple-sftpd, enabling secure FTP (FTPS) connections.

## 📋 Prerequisites

- simple-sftpd installed and configured
- Domain name pointing to your server
- Port 80 accessible for HTTP-01 challenge
- Root or sudo access
- Certbot installed

## 🚀 Quick Start

### 1. Install Certbot

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install certbot
```

#### CentOS/RHEL/Fedora
```bash
sudo dnf install certbot
# or for older versions
sudo yum install certbot
```

#### macOS
```bash
brew install certbot
```

#### Windows
```bash
# Download from https://certbot.eff.org/
# Or use WSL with Ubuntu
```

### 2. Obtain SSL Certificate

```bash
# Basic certificate for a single domain
sudo certbot certonly --standalone -d ftp.example.com

# Certificate for multiple domains
sudo certbot certonly --standalone -d ftp.example.com -d ftp2.example.com

# Wildcard certificate (requires DNS challenge)
sudo certbot certonly --manual --preferred-challenges dns -d *.example.com
```

### 3. Configure simple-sftpd SSL

```bash
# Copy certificates to simple-sftpd SSL directory
sudo mkdir -p /etc/simple-sftpd/ssl
sudo cp /etc/letsencrypt/live/ftp.example.com/fullchain.pem /etc/simple-sftpd/ssl/
sudo cp /etc/letsencrypt/live/ftp.example.com/privkey.pem /etc/simple-sftpd/ssl/

# Set proper permissions
sudo chown -R simple-sftpd:simple-sftpd /etc/simple-sftpd/ssl/
sudo chmod 600 /etc/simple-sftpd/ssl/*
```

## 🔧 Detailed Configuration

### HTTP-01 Challenge (Recommended)

The HTTP-01 challenge requires port 80 to be accessible:

```bash
# Stop any existing web server
sudo systemctl stop nginx
sudo systemctl stop apache2

# Obtain certificate
sudo certbot certonly --standalone -d ftp.example.com

# Restart web server if needed
sudo systemctl start nginx
```

### DNS Challenge (For Wildcards)

For wildcard certificates or when port 80 is not accessible:

```bash
# DNS challenge for wildcard
sudo certbot certonly --manual --preferred-challenges dns -d *.example.com

# Follow prompts to create TXT record
# Example TXT record: _acme-challenge.example.com = "abc123..."
```

### Staging vs Production

```bash
# Test with staging environment first
sudo certbot certonly --standalone --staging -d ftp.example.com

# Production certificate
sudo certbot certonly --standalone -d ftp.example.com
```

## 📁 Certificate File Locations

### Let's Encrypt Default
```
/etc/letsencrypt/live/ftp.example.com/
├── cert.pem          # Certificate
├── chain.pem         # Intermediate certificate
├── fullchain.pem     # Full certificate chain
└── privkey.pem       # Private key
```

### simple-sftpd SSL Directory
```
/etc/simple-sftpd/ssl/
├── ftp.example.com.crt    # Full certificate chain
├── ftp.example.com.key    # Private key
└── ca-bundle.crt          # CA bundle (optional)
```

## ⚙️ simple-sftpd SSL Configuration

### Basic SSL Module Configuration

```json
{
  "module": "ssl",
  "enabled": true,
  "priority": 10,
  "description": "SSL/TLS encryption support",
  
  "configuration": {
    "ssl_enabled": true,
    "ssl_port": 990,
    "require_ssl": false,
    "ssl_protocols": ["TLSv1.2", "TLSv1.3"]
  },
  
  "certificates": {
    "default": {
      "certificate_file": "/etc/simple-sftpd/ssl/ftp.example.com.crt",
      "private_key_file": "/etc/simple-sftpd/ssl/ftp.example.com.key"
    }
  }
}
```

### Virtual Host SSL Configuration

```json
{
  "site": "ftp.example.com",
  "enabled": true,
  "description": "Secure FTP site",
  
  "configuration": {
    "hostname": "ftp.example.com",
    "port": 21,
    "ssl_port": 990,
    "enable_ssl": true,
    "require_ssl": true
  },
  
  "ssl": {
    "certificate_file": "/etc/simple-sftpd/ssl/ftp.example.com.crt",
    "private_key_file": "/etc/simple-sftpd/ssl/ftp.example.com.key",
    "ca_bundle_file": "/etc/simple-sftpd/ssl/ca-bundle.crt"
  }
}
```

## 🔄 Certificate Renewal

### Automatic Renewal

```bash
# Test renewal
sudo certbot renew --dry-run

# Set up automatic renewal
sudo crontab -e

# Add this line for daily renewal checks
0 12 * * * /usr/bin/certbot renew --quiet
```

### Manual Renewal

```bash
# Renew specific certificate
sudo certbot renew --cert-name ftp.example.com

# Renew all certificates
sudo certbot renew
```

### Post-Renewal Script

Create a script to automatically update simple-sftpd after renewal:

```bash
#!/bin/bash
# /etc/letsencrypt/renewal-hooks/post/simple-sftpd-reload.sh

# Copy renewed certificates
sudo cp /etc/letsencrypt/live/ftp.example.com/fullchain.pem /etc/simple-sftpd/ssl/ftp.example.com.crt
sudo cp /etc/letsencrypt/live/ftp.example.com/privkey.pem /etc/simple-sftpd/ssl/ftp.example.com.key

# Set permissions
sudo chown simple-sftpd:simple-sftpd /etc/simple-sftpd/ssl/*
sudo chmod 600 /etc/simple-sftpd/ssl/*

# Reload simple-sftpd
sudo systemctl reload simple-sftpd

# Log renewal
echo "$(date): SSL certificate renewed for ftp.example.com" >> /var/log/simple-sftpd/ssl-renewal.log
```

Make it executable:
```bash
sudo chmod +x /etc/letsencrypt/renewal-hooks/post/simple-sftpd-reload.sh
```

## 🛠️ Management Commands

### Check Certificate Status

```bash
# List all certificates
sudo certbot certificates

# Check specific certificate
sudo certbot certificates --cert-name ftp.example.com

# Check expiration
openssl x509 -in /etc/simple-sftpd/ssl/ftp.example.com.crt -text -noout | grep "Not After"
```

### Revoke Certificate

```bash
# Revoke certificate
sudo certbot revoke --cert-path /etc/letsencrypt/live/ftp.example.com/cert.pem

# Delete certificate files
sudo certbot delete --cert-name ftp.example.com
```

### Update Certificate

```bash
# Force renewal
sudo certbot renew --force-renewal --cert-name ftp.example.com

# Update with new domains
sudo certbot certonly --standalone -d ftp.example.com -d ftp2.example.com
```

## 🔍 Troubleshooting

### Common Issues

#### Port 80 Already in Use
```bash
# Check what's using port 80
sudo netstat -tlnp | grep :80

# Stop conflicting service
sudo systemctl stop nginx
sudo systemctl stop apache2

# Or use DNS challenge instead
sudo certbot certonly --manual --preferred-challenges dns -d ftp.example.com
```

#### Permission Denied
```bash
# Fix SSL directory permissions
sudo chown -R simple-sftpd:simple-sftpd /etc/simple-sftpd/ssl/
sudo chmod 600 /etc/simple-sftpd/ssl/*
sudo chmod 700 /etc/simple-sftpd/ssl/
```

#### Certificate Not Found
```bash
# Verify certificate exists
sudo ls -la /etc/letsencrypt/live/ftp.example.com/

# Check symlinks
sudo ls -la /etc/letsencrypt/live/

# Recreate symlinks if needed
sudo certbot certificates
```

### Debug Mode

```bash
# Verbose output
sudo certbot certonly --standalone -d ftp.example.com --verbose

# Debug logs
sudo certbot certonly --standalone -d ftp.example.com --debug
```

## 📊 Monitoring and Logs

### Certificate Expiration Monitoring

```bash
# Check expiration dates
for cert in /etc/letsencrypt/live/*/; do
    if [[ -f "$cert/cert.pem" ]]; then
        echo "Certificate: $(basename $cert)"
        openssl x509 -in "$cert/cert.pem" -text -noout | grep "Not After"
    fi
done
```

### SSL Renewal Logs

```bash
# Check renewal logs
sudo tail -f /var/log/letsencrypt/letsencrypt.log

# Check simple-sftpd SSL logs
sudo tail -f /var/log/simple-sftpd/ssl.log
```

### Automated Monitoring Script

```bash
#!/bin/bash
# /usr/local/bin/check-ssl-expiry.sh

CERT_DIR="/etc/simple-sftpd/ssl"
DAYS_WARNING=30

for cert_file in "$CERT_DIR"/*.crt; do
    if [[ -f "$cert_file" ]]; then
        cert_name=$(basename "$cert_file" .crt)
        expiry_date=$(openssl x509 -in "$cert_file" -enddate -noout | cut -d= -f2)
        expiry_epoch=$(date -d "$expiry_date" +%s)
        current_epoch=$(date +%s)
        days_until_expiry=$(( (expiry_epoch - current_epoch) / 86400 ))
        
        if [[ $days_until_expiry -le $DAYS_WARNING ]]; then
            echo "WARNING: Certificate $cert_name expires in $days_until_expiry days"
            # Send notification email or alert
        fi
    fi
done
```

## 🔐 Security Best Practices

### Certificate Security

```bash
# Restrict access to private keys
sudo chmod 600 /etc/simple-sftpd/ssl/*.key
sudo chown simple-sftpd:simple-sftpd /etc/simple-sftpd/ssl/*.key

# Use strong key algorithms
# Certbot automatically uses RSA 2048-bit or ECDSA P-256
```

### SSL Configuration

```json
{
  "ssl_protocols": ["TLSv1.2", "TLSv1.3"],
  "ssl_ciphers": [
    "ECDHE-RSA-AES256-GCM-SHA384",
    "ECDHE-RSA-AES128-GCM-SHA256"
  ],
  "ssl_prefer_server_ciphers": true,
  "ssl_session_cache": {
    "enabled": true,
    "size": 1024,
    "timeout": 300
  }
}
```

### HSTS and Security Headers

```json
{
  "security": {
    "hsts_enabled": true,
    "hsts_max_age": 31536000,
    "hsts_include_subdomains": true,
    "hsts_preload": false
  }
}
```

## 📚 Additional Resources

### Let's Encrypt Documentation
- [Getting Started](https://letsencrypt.org/getting-started/)
- [Challenge Types](https://letsencrypt.org/docs/challenge-types/)
- [Rate Limits](https://letsencrypt.org/docs/rate-limits/)

### simple-sftpd SSL Documentation
- [SSL Module Configuration](../modules-available/ssl.conf)
- [Virtual Host SSL Setup](../sites-available/)
- [SSL Troubleshooting](../troubleshooting.md)

### Community Support
- [GitHub Issues](https://github.com/blburns/simple-sftpd/issues)
- [Community Forum](https://community.simple-sftpd.org)
- [Discord Server](https://discord.gg/simple-sftpd)

---

**Next Steps**: 
1. Install Certbot on your system
2. Obtain your first SSL certificate
3. Configure simple-sftpd to use the certificate
4. Set up automatic renewal
5. Test FTPS connections
