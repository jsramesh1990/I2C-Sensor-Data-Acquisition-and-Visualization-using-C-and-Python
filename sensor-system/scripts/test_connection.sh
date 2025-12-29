#!/bin/bash

# Sensor Monitoring System - Connection Tester
# Tests communication between backend and GUI

set -e

echo "=== Testing Sensor System Connection ==="

# Test 1: Check if backend is running
echo "Test 1: Checking if backend is running..."
if [ -S "/tmp/sensor_system.sock" ]; then
    echo "✓ Backend socket found"
    
    # Test socket connection
    if echo "test" | nc -U /tmp/sensor_system.sock 2>/dev/null | grep -q .; then
        echo "✓ Backend is responding"
    else
        echo "✗ Backend is not responding"
    fi
else
    echo "✗ Backend socket not found"
fi

echo ""

# Test 2: Check database
echo "Test 2: Checking database..."
if [ -f "data/sensor_data.db" ]; then
    echo "✓ Database file exists"
    size=$(stat -f%z "data/sensor_data.db" 2>/dev/null || stat -c%s "data/sensor_data.db")
    echo "  Size: $size bytes"
    
    # Check if we can read the database
    if command -v sqlite3 &> /dev/null; then
        if echo ".tables" | sqlite3 data/sensor_data.db 2>/dev/null | grep -q .; then
            echo "✓ Database is readable"
        else
            echo "✗ Cannot read database"
        fi
    fi
else
    echo "✗ Database file not found"
fi

echo ""

# Test 3: Check Python dependencies
echo "Test 3: Checking Python dependencies..."
if command -v python3 &> /dev/null; then
    echo "✓ Python3 found"
    
    # Test imports
    if python3 -c "import PyQt5, matplotlib, numpy" 2>/dev/null; then
        echo "✓ All Python dependencies satisfied"
    else
        echo "✗ Missing Python dependencies"
        python3 -c "import PyQt5" 2>/dev/null || echo "  - PyQt5"
        python3 -c "import matplotlib" 2>/dev/null || echo "  - matplotlib"
        python3 -c "import numpy" 2>/dev/null || echo "  - numpy"
    fi
else
    echo "✗ Python3 not found"
fi

echo ""

# Test 4: Check compilation
echo "Test 4: Checking backend compilation..."
if [ -f "bin/sensor_backend" ]; then
    echo "✓ Backend binary exists"
    
    # Check if it's executable
    if [ -x "bin/sensor_backend" ]; then
        echo "✓ Backend is executable"
        
        # Test run
        timeout 1 ./bin/sensor_backend --help 2>/dev/null && \
            echo "✓ Backend runs correctly" || \
            echo "✗ Backend has issues running"
    else
        echo "✗ Backend is not executable"
    fi
else
    echo "✗ Backend binary not found"
    echo "  Run 'make' to build"
fi

echo ""
echo "=== Connection Test Complete ==="
echo ""
echo "Summary:"
echo "1. Run './scripts/start_backend.sh' to start backend"
echo "2. Run './scripts/start_gui.sh' to start GUI"
echo "3. Check 'logs/backend.log' for backend issues"
