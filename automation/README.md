# simple-sftpd - Automation

This directory contains all automation scripts and configuration files for setting up and managing the simple-sftpd development environment.

## Product Versions

The automation system supports building three product versions:
- **Production** (default): Apache 2.0 licensed, basic features
- **Enterprise**: BSL 1.1 licensed, includes production features plus enterprise features
- **Datacenter**: BSL 1.1 licensed, includes all enterprise and production features plus datacenter features

Build a specific version by setting the `build_version` variable:
```bash
ansible-playbook -i automation/ansible/inventory.ini automation/ansible/playbook-build.yml -e build_version=enterprise
```

Packages are organized by product version in `dist/centralized/v{VERSION}/{PRODUCT_VERSION}/`:
- `dist/centralized/v0.3.0/production/` - Production packages
- `dist/centralized/v0.3.0/enterprise/` - Enterprise packages (when built)
- `dist/centralized/v0.3.0/datacenter/` - Datacenter packages (when built)

Package naming format: `simple-sftpd-{version}-{product}-{platform}-{distro}-{arch}.{ext}`  
Example: `simple-sftpd-0.3.0-production-linux-debian-amd64.deb`

## Directory Structure

```
automation/
├── ansible/                  # Ansible automation
│   ├── playbook.yml         # Ansible playbook for VM setup
│   ├── inventory.ini        # Ansible inventory file
│   ├── requirements.yml     # Ansible Galaxy requirements
│   ├── Makefile.vm          # Makefile for VM operations
│   ├── vagrant-boxes.yml   # Vagrant box configurations
│   ├── scripts/             # Shell scripts for VM operations
│   │   ├── vm-ssh          # SSH wrapper for VM
│   │   ├── vm-build        # Build script for VM
│   │   ├── vm-test         # Test script for VM
│   │   ├── setup-remote.sh # Remote setup script
│   │   └── build.sh        # Build script
│   └── templates/          # Configuration templates
├── ci/                      # CI/CD configuration
│   ├── Jenkinsfile         # Jenkins pipeline
│   └── .travis.yml         # Travis CI configuration
├── docker/                  # Docker configuration
│   ├── Dockerfile          # Docker image definition
│   ├── docker-compose.yml  # Docker Compose configuration
│   └── examples/           # Docker examples
└── vagrant/                 # Vagrant configuration
    ├── Vagrantfile         # Main Vagrantfile
    └── virtuals/           # Multi-VM configurations
        ├── ubuntu_dev/
        └── centos_dev/
```

## Quick Start

### Using Docker

```bash
# Build and run with Docker Compose
cd automation/docker
docker-compose up -d

# Or from project root
docker-compose -f automation/docker/docker-compose.yml up -d
```

### Using Vagrant

```bash
# Start VM
cd automation/vagrant
vagrant up

# SSH into VM
vagrant ssh

# Build project
./automation/ansible/scripts/vm-build
```

### Using Ansible

```bash
# Run playbook
ansible-playbook -i automation/ansible/inventory.ini automation/ansible/playbook.yml
```

## CI/CD

### Jenkins

The Jenkins pipeline is located at `automation/ci/Jenkinsfile`. It supports:
- Multi-platform builds (Linux, macOS, Windows)
- Automated testing
- Static analysis
- Package generation
- Docker image building

### Travis CI

The Travis CI configuration is located at `automation/ci/.travis.yml`. It provides:
- Automated builds on push
- Multi-platform testing
- Code coverage reporting

## Docker

Docker files are located in `automation/docker/`:
- `Dockerfile` - Multi-stage build for different distributions
- `docker-compose.yml` - Development and production configurations
- `examples/` - Example Docker configurations

## Vagrant

Vagrant configuration is in `automation/vagrant/`:
- `Vagrantfile` - Main Vagrant configuration
- `virtuals/` - Multi-VM configurations for different distributions

## macOS build VM: shared Homebrew

Do **not** `chown -R build /usr/local/Homebrew` — that breaks other users' Homebrew.

**Recommended (multi-user):** run once on the macOS build VM as admin:

```bash
sudo automation/ansible/scripts/setup-macos-homebrew-shared.sh admin build
```

That creates a `homebrew` group, adds both users, and makes the install group-writable. Everyone in the group keeps using the same `brew`.

Users must log out/in (or `newgrp homebrew`) after being added to the group.

**Alternative:** if `build` has passwordless `sudo` to the Homebrew owner, set in inventory:

```ini
BUILD_MACOS ... homebrew_run_as=admin
```

Ansible will run `brew` as `admin` while still connecting as `build`.

## Ansible

Ansible automation is in `automation/ansible/`:
- `playbook.yml` - Main playbook for environment setup
- `inventory.ini` - Host inventory
- `requirements.yml` - Ansible Galaxy dependencies
- `scripts/` - Helper scripts for VM operations
- `templates/` - Configuration templates

---

*For detailed documentation, see the individual README files in each subdirectory.*

