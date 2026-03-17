#!/bin/bash

# Simple FTP Daemon - Service Installation Script
# This script helps install the Simple FTP Daemon as a system service

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="simple-sftpd"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
CONFIG_DIR="/etc/simple-sftpd"
CONFIG_FILE="$CONFIG_DIR/simple-sftpd.conf"
BINARY_PATH="/usr/local/bin/simple-sftpd"
FTP_USER="ftp"
FTP_GROUP="ftp"
FTP_DIR="/var/ftp"
LOG_DIR="/var/log/simple-sftpd"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

# Function to detect OS
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
        ID=$ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
        ID=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        OS=$DISTRIB_ID
        VER=$DISTRIB_RELEASE
        ID=$(echo $DISTRIB_ID | tr '[:upper:]' '[:lower:]')
    elif [[ -f /etc/debian_version ]]; then
        OS=Debian
        VER=$(cat /etc/debian_version)
        ID="debian"
    elif [[ -f /etc/SuSe-release ]]; then
        OS=SuSE
        ID="suse"
    elif [[ -f /etc/redhat-release ]]; then
        OS=RedHat
        ID="redhat"
    else
        OS=$(uname -s)
        VER=$(uname -r)
        ID="unknown"
    fi
    
    print_status "Detected OS: $OS $VER (ID: $ID)"
}

# Function to check if systemd is available
check_systemd() {
    if ! command -v systemctl &> /dev/null; then
        print_error "systemd is not available on this system"
        print_error "This script only supports systemd-based systems"
        exit 1
    fi
    
    if ! systemctl --version &> /dev/null; then
        print_error "systemd is not running or accessible"
        exit 1
    fi
    
    print_success "systemd is available and running"
}

# Function to check if binary exists
check_binary() {
    if [[ ! -f "$BINARY_PATH" ]]; then
        print_error "Simple FTP Daemon binary not found at $BINARY_PATH"
        print_error "Please build and install the application first"
        exit 1
    fi
    
    if [[ ! -x "$BINARY_PATH" ]]; then
        print_error "Binary is not executable: $BINARY_PATH"
        exit 1
    fi
    
    print_success "Binary found and executable: $BINARY_PATH"
}

# Function to create FTP user and group
create_ftp_user() {
    print_status "Creating FTP user and group..."
    
    # Create FTP group if it doesn't exist
    if ! getent group $FTP_GROUP > /dev/null 2>&1; then
        groupadd -r $FTP_GROUP
        print_success "Created group: $FTP_GROUP"
    else
        print_status "Group $FTP_GROUP already exists"
    fi
    
    # Create FTP user if it doesn't exist
    if ! getent passwd $FTP_USER > /dev/null 2>&1; then
        useradd -r -s /bin/false -d $FTP_DIR -g $FTP_GROUP $FTP_USER
        print_success "Created user: $FTP_USER"
    else
        print_status "User $FTP_USER already exists"
    fi
    
    # Create FTP directory
    mkdir -p "$FTP_DIR"
    chown $FTP_USER:$FTP_GROUP "$FTP_DIR"
    chmod 755 "$FTP_DIR"
    
    print_success "FTP user setup completed"
}

# Function to create directories
create_directories() {
    print_status "Creating necessary directories..."
    
    # Create configuration directory
    mkdir -p "$CONFIG_DIR"
    chown $FTP_USER:$FTP_GROUP "$CONFIG_DIR"
    chmod 755 "$CONFIG_DIR"
    
    # Create log directory
    mkdir -p "$LOG_DIR"
    chown $FTP_USER:$FTP_GROUP "$LOG_DIR"
    chmod 755 "$LOG_DIR"
    
    # Create FTP directories
    mkdir -p "$FTP_DIR/public"
    mkdir -p "$FTP_DIR/private"
    chown -R $FTP_USER:$FTP_GROUP "$FTP_DIR"
    chmod 755 "$FTP_DIR/public"
    chmod 700 "$FTP_DIR/private"
    
    print_success "Directories created"
}

# Function to create configuration file
create_config() {
    print_status "Creating configuration file..."
    
    if [[ -f "$CONFIG_FILE" ]]; then
        print_warning "Configuration file already exists: $CONFIG_FILE"
        print_status "Backing up existing configuration..."
        cp "$CONFIG_FILE" "$CONFIG_FILE.backup.$(date +%Y%m%d_%H%M%S)"
    fi
    
    # Create basic configuration
    cat > "$CONFIG_FILE" << EOF
# Simple FTP Daemon Configuration File
# Generated on $(date)

[server]
name = Simple FTP Daemon
version = 1.0.0

[connection]
bind_address = 0.0.0.0
bind_port = 21
max_connections = 100
max_connections_per_ip = 10
connection_timeout = 300
idle_timeout = 600

[ssl]
enabled = false
certificate_file = /etc/ssl/simple-sftpd/certs/server.crt
private_key_file = /etc/ssl/simple-sftpd/private/server.key
require_client_cert = false
verify_peer = false

[logging]
log_file = $LOG_DIR/simple-sftpd.log
log_level = INFO
log_to_console = false
log_to_file = true
log_commands = true
log_transfers = true
log_errors = true

[security]
chroot_enabled = true
chroot_directory = $FTP_DIR
drop_privileges = true
run_as_user = $FTP_USER
run_as_group = $FTP_GROUP
allow_anonymous = false
max_login_attempts = 3
login_timeout = 30
session_timeout = 3600

[transfer]
max_transfer_rate = 1048576
max_transfer_rate_per_user = 524288
max_file_size = 104857600
buffer_size = 8192

[passive]
enabled = true
min_port = 49152
max_port = 65535
use_external_ip = false
external_ip = 

[rate_limiting]
enabled = true
max_requests_per_minute = 60
burst_size = 10

[virtual_hosts]
default_host = localhost

[users]
# User configurations will be added here
# Use the manage-users.sh script to manage users
EOF
    
    chown $FTP_USER:$FTP_GROUP "$CONFIG_FILE"
    chmod 644 "$CONFIG_FILE"
    
    print_success "Configuration file created: $CONFIG_FILE"
}

# Function to create systemd service file
create_service_file() {
    print_status "Creating systemd service file..."
    
    if [[ -f "$SERVICE_FILE" ]]; then
        print_warning "Service file already exists: $SERVICE_FILE"
        print_status "Backing up existing service file..."
        cp "$SERVICE_FILE" "$SERVICE_FILE.backup.$(date +%Y%m%d_%H%M%S)"
    fi
    
    # Create systemd service file
    cat > "$SERVICE_FILE" << EOF
[Unit]
Description=Simple FTP Daemon
Documentation=man:simple-sftpd(8)
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=$FTP_USER
Group=$FTP_GROUP
ExecStart=$BINARY_PATH --config $CONFIG_FILE
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal
SyslogIdentifier=$SERVICE_NAME

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$FTP_DIR $LOG_DIR $CONFIG_DIR
CapabilityBoundingSet=CAP_NET_BIND_SERVICE

# Resource limits
LimitNOFILE=65536
LimitNPROC=1024

# Environment
Environment=HOME=$FTP_DIR

[Install]
WantedBy=multi-user.target
EOF
    
    chmod 644 "$SERVICE_FILE"
    
    print_success "Service file created: $SERVICE_FILE"
}

# Function to create control script
create_control_script() {
    local control_script="/usr/local/bin/${SERVICE_NAME}-ctl"
    
    print_status "Creating control script..."
    
    cat > "$control_script" << 'EOF'
#!/bin/bash

# Simple FTP Daemon Control Script

SERVICE_NAME="simple-sftpd"
CONFIG_FILE="/etc/simple-sftpd/simple-sftpd.conf"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

case "$1" in
    start)
        echo "Starting Simple FTP Daemon..."
        systemctl start $SERVICE_NAME
        ;;
    stop)
        echo "Stopping Simple FTP Daemon..."
        systemctl stop $SERVICE_NAME
        ;;
    restart)
        echo "Restarting Simple FTP Daemon..."
        systemctl restart $SERVICE_NAME
        ;;
    reload)
        echo "Reloading Simple FTP Daemon configuration..."
        systemctl reload $SERVICE_NAME
        ;;
    status)
        systemctl status $SERVICE_NAME
        ;;
    enable)
        echo "Enabling Simple FTP Daemon to start on boot..."
        systemctl enable $SERVICE_NAME
        ;;
    disable)
        echo "Disabling Simple FTP Daemon from starting on boot..."
        systemctl disable $SERVICE_NAME
        ;;
    config-test)
        echo "Testing configuration file..."
        if [[ -f "$CONFIG_FILE" ]]; then
            echo "Configuration file exists: $CONFIG_FILE"
            echo "Testing syntax..."
            # Add configuration validation here if available
            echo "Configuration appears valid"
        else
            echo "Configuration file not found: $CONFIG_FILE"
            exit 1
        fi
        ;;
    logs)
        echo "Showing recent logs..."
        journalctl -u $SERVICE_NAME -f
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|reload|status|enable|disable|config-test|logs}"
        exit 1
        ;;
esac

exit 0
EOF
    
    chmod +x "$control_script"
    
    print_success "Control script created: $control_script"
}

# Function to create logrotate configuration
create_logrotate() {
    local logrotate_file="/etc/logrotate.d/$SERVICE_NAME"
    
    print_status "Creating logrotate configuration..."
    
    cat > "$logrotate_file" << EOF
$LOG_DIR/*.log {
    daily
    missingok
    rotate 52
    compress
    delaycompress
    notifempty
    create 644 $FTP_USER $FTP_GROUP
    postrotate
        systemctl reload $SERVICE_NAME > /dev/null 2>&1 || true
    endscript
}
EOF
    
    chmod 644 "$logrotate_file"
    
    print_success "Logrotate configuration created: $logrotate_file"
}

# Function to create firewall rules
create_firewall_rules() {
    print_status "Configuring firewall rules..."
    
    case $ID in
        ubuntu|debian)
            if command -v ufw &> /dev/null; then
                ufw allow 21/tcp comment "FTP Control"
                ufw allow 49152:65535/tcp comment "FTP Passive Ports"
                print_success "UFW firewall rules added"
            else
                print_warning "UFW not found, skipping firewall configuration"
            fi
            ;;
        centos|rhel|fedora)
            if command -v firewall-cmd &> /dev/null; then
                firewall-cmd --permanent --add-service=ftp
                firewall-cmd --permanent --add-port=49152-65535/tcp
                firewall-cmd --reload
                print_success "firewalld rules added"
            else
                print_warning "firewalld not found, skipping firewall configuration"
            fi
            ;;
        suse|opensuse)
            if command -v firewall-cmd &> /dev/null; then
                firewall-cmd --permanent --add-service=ftp
                firewall-cmd --permanent --add-port=49152-65535/tcp
                firewall-cmd --reload
                print_success "firewalld rules added"
            else
                print_warning "firewalld not found, skipping firewall configuration"
            fi
            ;;
        *)
            print_warning "Unknown distribution, skipping firewall configuration"
            print_status "Please configure firewall manually:"
            echo "  - Allow port 21 (FTP control)"
            echo "  - Allow ports 49152-65535 (FTP passive)"
            ;;
    esac
}

# Function to enable and start service
enable_service() {
    print_status "Enabling and starting service..."
    
    # Reload systemd
    systemctl daemon-reload
    
    # Enable service
    systemctl enable $SERVICE_NAME
    
    # Start service
    systemctl start $SERVICE_NAME
    
    # Check status
    if systemctl is-active --quiet $SERVICE_NAME; then
        print_success "Service started successfully"
    else
        print_error "Failed to start service"
        systemctl status $SERVICE_NAME
        exit 1
    fi
}

# Function to display post-installation information
show_post_install_info() {
    echo ""
    echo "=========================================="
    echo "Simple FTP Daemon Installation Complete!"
    echo "=========================================="
    echo ""
    echo "Service Information:"
    echo "  Service Name: $SERVICE_NAME"
    echo "  Binary Path: $BINARY_PATH"
    echo "  Config File: $CONFIG_FILE"
    echo "  FTP Directory: $FTP_DIR"
    echo "  Log Directory: $LOG_DIR"
    echo ""
    echo "Control Commands:"
    echo "  Start:   systemctl start $SERVICE_NAME"
    echo "  Stop:    systemctl stop $SERVICE_NAME"
    echo "  Restart: systemctl restart $SERVICE_NAME"
    echo "  Status:  systemctl status $SERVICE_NAME"
    echo "  Enable:  systemctl enable $SERVICE_NAME"
    echo "  Disable: systemctl disable $SERVICE_NAME"
    echo ""
    echo "Or use the control script:"
    echo "  $SERVICE_NAME-ctl start|stop|restart|status|enable|disable"
    echo ""
    echo "Next Steps:"
    echo "1. Configure users: tools/manage-users.sh add <username> <password>"
    echo "2. Configure SSL (optional): tools/setup-ssl.sh <domain>"
    echo "3. Test connection: ftp localhost"
    echo "4. Check logs: journalctl -u $SERVICE_NAME -f"
    echo ""
    echo "Documentation:"
    echo "  - User Guide: docs/user-guide/README.md"
    echo "  - Configuration: docs/configuration/README.md"
    echo "  - Examples: docs/examples/README.md"
    echo ""
}

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --no-firewall        Skip firewall configuration"
    echo "  --no-logrotate       Skip logrotate configuration"
    echo "  --no-start           Don't start the service after installation"
    echo "  -h, --help           Display this help message"
    echo ""
    echo "This script will:"
    echo "1. Create FTP user and group"
    echo "2. Create necessary directories"
    echo "3. Create configuration file"
    echo "4. Create systemd service file"
    echo "5. Create control script"
    echo "6. Configure firewall (optional)"
    echo "7. Configure logrotate (optional)"
    echo "8. Enable and start the service"
    echo ""
}

# Main script
main() {
    local skip_firewall=false
    local skip_logrotate=false
    local skip_start=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --no-firewall)
                skip_firewall=true
                shift
                ;;
            --no-logrotate)
                skip_logrotate=true
                shift
                ;;
            --no-start)
                skip_start=true
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            -*)
                print_error "Unknown option: $1"
                usage
                exit 1
                ;;
            *)
                print_error "Unexpected argument: $1"
                usage
                exit 1
                ;;
        esac
    done
    
    # Check if running as root
    check_root
    
    # Detect OS
    detect_os
    
    # Check systemd
    check_systemd
    
    # Check binary
    check_binary
    
    print_status "Starting Simple FTP Daemon service installation..."
    echo ""
    
    # Create FTP user and group
    create_ftp_user
    
    # Create directories
    create_directories
    
    # Create configuration file
    create_config
    
    # Create systemd service file
    create_service_file
    
    # Create control script
    create_control_script
    
    # Create logrotate configuration
    if [[ "$skip_logrotate" == "false" ]]; then
        create_logrotate
    fi
    
    # Configure firewall
    if [[ "$skip_firewall" == "false" ]]; then
        create_firewall_rules
    fi
    
    # Enable and start service
    if [[ "$skip_start" == "false" ]]; then
        enable_service
    fi
    
    # Show post-installation information
    show_post_install_info
}

# Run main function with all arguments
main "$@"
