#!/bin/bash
# Post-uninstallation script for simple-sftpd RPM

set -e

SERVICE=simple-sftpd

if command -v systemctl >/dev/null 2>&1; then
    systemctl stop "${SERVICE}.service" 2>/dev/null || true
    systemctl disable "${SERVICE}.service" 2>/dev/null || true
    systemctl daemon-reload
fi

exit 0
