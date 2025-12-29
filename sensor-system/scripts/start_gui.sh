#!/bin/bash

# Sensor Monitoring System - GUI Launcher
# Starts the Python GUI application

set -e

echo "=== Starting Sensor Monitoring GUI ==="

# Check if Python3 is available
if ! command -v python3 &> /dev/null; then
    echo "Python3 not found. Please install Python3."
    exit 1
fi

# Check if backend is running
if [ ! -S "/tmp/sensor_system.sock" ]; then
    echo "Warning: Backend not detected."
    echo "The GUI will start but won't be able to connect."
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check Python dependencies
echo "Checking Python dependencies..."
python3 -c "import PyQt5, matplotlib" 2>/dev/null || {
    echo "Missing Python dependencies."
    echo "Installing from requirements.txt..."
    pip3 install -r requirements.txt
}

# Start GUI
echo "Starting GUI..."
cd "$(dirname "$0")/.."
python3 src/gui/gui.py

# Exit with GUI's exit code
exit $?
