#!/bin/bash

# Sensor Monitoring System - Backend Stopper
# Stops the backend gracefully

echo "=== Stopping Sensor Monitoring Backend ==="

# Check PID file
if [ -f "/tmp/sensor_backend.pid" ]; then
    BACKEND_PID=$(cat /tmp/sensor_backend.pid)
    
    if kill -0 $BACKEND_PID 2>/dev/null; then
        echo "Stopping backend (PID: $BACKEND_PID)..."
        kill $BACKEND_PID
        
        # Wait for process to end
        for i in {1..10}; do
            if ! kill -0 $BACKEND_PID 2>/dev/null; then
                echo "Backend stopped successfully"
                rm -f /tmp/sensor_backend.pid
                rm -f /tmp/sensor_system.sock
                exit 0
            fi
            sleep 1
        done
        
        # Force kill if still running
        echo "Backend not responding to SIGTERM, forcing kill..."
        kill -9 $BACKEND_PID
        rm -f /tmp/sensor_backend.pid
        rm -f /tmp/sensor_system.sock
        echo "Backend force stopped"
    else
        echo "Backend not running (PID: $BACKEND_PID)"
        rm -f /tmp/sensor_backend.pid
    fi
else
    echo "No PID file found. Trying to find and stop backend..."
    
    # Try to find by socket
    if [ -S "/tmp/sensor_system.sock" ]; then
        echo "Found socket, but no PID file."
        echo "You may need to manually stop the backend:"
        echo "  pkill sensor_backend"
        echo "  rm -f /tmp/sensor_system.sock"
    else
        echo "Backend doesn't appear to be running"
    fi
fi
