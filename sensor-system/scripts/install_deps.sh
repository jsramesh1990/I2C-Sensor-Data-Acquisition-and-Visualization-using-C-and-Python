#!/bin/bash

# Sensor Monitoring System - Dependency Installer
# This script installs all necessary dependencies

set -e

echo "=== Installing Sensor Monitoring System Dependencies ==="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root or with sudo"
    exit 1
fi

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot detect OS"
    exit 1
fi

echo "Detected OS: $OS"

# Install dependencies based on OS
case $OS in
    ubuntu|debian)
        echo "Installing for Debian/Ubuntu..."
        apt-get update
        apt-get install -y \
            build-essential \
            gcc \
            make \
            python3 \
            python3-pip \
            python3-pyqt5 \
            python3-matplotlib \
            libsqlite3-dev \
            pkg-config
        ;;
    
    fedora|centos|rhel)
        echo "Installing for Fedora/RHEL/CentOS..."
        if [ "$OS" = "fedora" ]; then
            dnf install -y \
                gcc \
                make \
                python3 \
                python3-pip \
                python3-qt5 \
                python3-matplotlib \
                sqlite-devel \
                gcc-c++
        else
            yum install -y \
                gcc \
                make \
                python3 \
                python3-pip \
                qt5-qtbase-devel \
                python3-matplotlib \
                sqlite-devel \
                gcc-c++
        fi
        ;;
    
    arch)
        echo "Installing for Arch Linux..."
        pacman -Sy --noconfirm \
            base-devel \
            gcc \
            make \
            python \
            python-pip \
            python-pyqt5 \
            python-matplotlib \
            sqlite
        ;;
    
    *)
        echo "Unsupported OS: $OS"
        echo "Please install dependencies manually:"
        echo "- C compiler (gcc)"
        echo "- Make"
        echo "- Python 3"
        echo "- PyQt5"
        echo "- Matplotlib"
        echo "- SQLite development libraries"
        exit 1
        ;;
esac

# Install Python packages
echo "Installing Python packages..."
pip3 install PyQt5 matplotlib numpy

# Create necessary directories
echo "Creating directories..."
mkdir -p data
mkdir -p logs

# Set permissions
echo "Setting permissions..."
chmod +x scripts/*.sh

echo ""
echo "=== Dependencies installed successfully! ==="
echo ""
echo "Next steps:"
echo "1. Build the backend: make"
echo "2. Start the backend: ./scripts/start_backend.sh"
echo "3. Start the GUI: ./scripts/start_gui.sh"
