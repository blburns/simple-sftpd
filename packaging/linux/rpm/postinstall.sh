#!/bin/bash
# Post-installation script for simple-sftpd RPM

set -e

SERVICE=simple-sftpd
SERVICE_USER=simple-sftpd
SERVICE_GROUP=simple-sftpd

if ! getent passwd "$SERVICE_USER" >/dev/null 2>&1; then
    useradd --system --home-dir /var/lib/simple-sftpd --shell /sbin/nologin \
        --comment "Simple Secure FTP Daemon" "$SERVICE_USER"
fi

mkdir -p /var/ftp /var/log/simple-sftpd /var/lib/simple-sftpd
chown "$SERVICE_USER:$SERVICE_GROUP" /var/ftp /var/log/simple-sftpd /var/lib/simple-sftpd
chmod 755 /etc/simple-sftpd 2>/dev/null || true

if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload
    systemctl enable "${SERVICE}.service" || true
    systemctl try-restart "${SERVICE}.service" || true
fi

exit 0
