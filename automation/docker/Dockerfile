# Multi-stage Dockerfile for cross-platform building
# Supports: Ubuntu, Debian, CentOS, Alpine
# Architectures: x86_64, arm64, armv7

# Base builder image
FROM ubuntu:22.04 AS base-builder

# Install common build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu/Debian builder
FROM base-builder AS ubuntu-builder

# Install Ubuntu-specific dependencies
RUN apt-get update && apt-get install -y \
    libssl-dev \
    libjsoncpp-dev \
    dpkg-dev \
    && rm -rf /var/lib/apt/lists/*

# Set build environment
ENV PLATFORM=linux
ENV DISTRO=ubuntu
ENV ARCH=x86_64

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Create package
RUN make package

# CentOS/RHEL builder
FROM base-builder AS centos-builder

# Install CentOS-specific dependencies
RUN yum update -y && yum install -y \
    gcc-c++ \
    cmake3 \
    openssl-devel \
    jsoncpp-devel \
    pkgconfig \
    rpm-build \
    && yum clean all

# Set build environment
ENV PLATFORM=linux
ENV DISTRO=centos
ENV ARCH=x86_64

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build && cd build \
    && cmake3 .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Create package
RUN make package

# Alpine Linux builder
FROM alpine:3.18 AS alpine-builder

# Install Alpine-specific dependencies
RUN apk add --no-cache \
    build-base \
    cmake \
    pkgconfig \
    openssl-dev \
    jsoncpp-dev \
    git

# Set build environment
ENV PLATFORM=linux
ENV DISTRO=alpine
ENV ARCH=x86_64

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Create package
RUN make package

# Multi-architecture builder using buildx
FROM --platform=$BUILDPLATFORM base-builder AS multiarch-builder

# Install cross-compilation tools
RUN apt-get update && apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    && rm -rf /var/lib/apt/lists/*

# Set build environment
ENV PLATFORM=linux
ENV DISTRO=multiarch
ENV TARGETPLATFORM=$TARGETPLATFORM

# Copy source code
WORKDIR /app
COPY . .

# Build for target architecture
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# Runtime image (minimal)
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    libjsoncpp25 \
    ca-certificates \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -r -s /bin/false simple-sftpd

# Create FTP directories
RUN mkdir -p /var/ftp/pub /var/ftp/incoming /var/ftp/outgoing \
    && chown -R simple-sftpd:simple-sftpd /var/ftp

# Copy binary from builder
COPY --from=ubuntu-builder /app/build/bin/simple-sftpd /usr/local/bin/
COPY --from=ubuntu-builder /app/config/ /etc/simple-sftpd/

# Set ownership
RUN chown -R simple-sftpd:simple-sftpd /etc/simple-sftpd

# Expose FTP ports (21 for FTP, 990 for FTPS, passive mode range)
EXPOSE 21/tcp 990/tcp 1024-65535/tcp

# Switch to non-root user
USER simple-sftpd

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD nc -z localhost 21 || exit 1

# Default command
CMD ["simple-sftpd", "--config", "/etc/simple-sftpd/simple-sftpd.conf"]

# Development image
FROM base-builder AS dev

# Install development tools
RUN apt-get update && apt-get install -y \
    libssl-dev \
    libjsoncpp-dev \
    clang-format \
    cppcheck \
    valgrind \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Install Python tools
RUN apt-get update && apt-get install -y \
    python3-pip \
    && pip3 install bandit semgrep

# Set development environment
ENV PLATFORM=linux
ENV DISTRO=ubuntu
ENV ARCH=x86_64

# Copy source code
WORKDIR /app
COPY . .

# Default command for development
CMD ["/bin/bash"]
