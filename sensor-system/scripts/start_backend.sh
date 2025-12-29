#!/bin/bash

# Sensor Monitoring System - Backend Launcher
# Starts the C backend application

set -e

echo "=== Starting Sensor Monitoring Backend ==="

# Check if backend is built
if [ ! -f "bin/sensor_backend" ]; then
    echo "Backend not found. Building..."
    make
fi

# Check if already running
if [ -S "/tmp/sensor_system.sock" ]; then
    echo "Backend might already be running."
    echo "If not, remove /tmp/sensor_system.sock and try again."
    read -p "Force start? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
    rm -f /tmp/sensor_system.sock
fi

# Create log directory
mkdir -p logs

# Start backend with logging
echo "Starting backend..."
echo "Logs will be written to logs/backend.log"

# Run in background with logging
nohup ./bin/sensor_backend > logs/backend.log 2>&1 &
BACKEND_PID=$!

# Wait a bit for startup
sleep 2

# Check if started successfully
if kill -0 $BACKEND_PID 2>/dev/null; then
    echo "Backend started successfully (PID: $BACKEND_PID)"
    echo "Socket: /tmp/sensor_system.sock"
    
    # Create PID file
    echo $BACKEND_PID > /tmp/sensor_backend.pid
    
    # Show tail of log
    echo ""
    echo "=== Last 5 lines of log ==="
    tail -5 logs/backend.log
    
    echo ""
    echo "Backend is running. Use './scripts/stop_backend.sh' to stop."
else
    echo "Failed to start backend"
    echo "Check logs/backend.log for details"
    exit 1
fi
