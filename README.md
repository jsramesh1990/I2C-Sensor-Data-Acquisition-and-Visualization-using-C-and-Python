# Real-Time I²C Sensor Data Acquisition and Visualization  
### Hybrid Embedded Data Processing Framework (C Backend + Python GUI)

## Overview
This project demonstrates a **hybrid embedded data processing system** that combines:
- **C-based backend** for real-time data acquisition, multithreading, inter-process communication (IPC), and SQLite data storage.
- **Python-based GUI (PyQt5 + Matplotlib)** for real-time data visualization.

It simulates **I²C sensor readings** (temperature and humidity), transfers data to the GUI using **UNIX domain sockets**, and visualizes live graphs.


#Features
✅ Real-time sensor data simulation using sine/cosine patterns  
✅ Multithreaded backend written in C (POSIX threads)  
✅ IPC via UNIX domain sockets for data exchange  
✅ SQLite database logging  
✅ PyQt5 GUI visualization with Matplotlib  
✅ Modular architecture — replace simulated sensors with real I²C easily  


**Build and Run**
sudo apt update
sudo apt install libsqlite3-dev

Then recompile:

make clean
make

sudo apt update
sudo apt install python3-pyqt5 python3-matplotlib

pip install PyQt5 matplotlib

python3 gui.py

**How It Works**

Backend

Simulates I²C sensor readings using sin() and cos() functions.

Writes readings to SQLite database.

Sends data over a UNIX socket to the GUI process.

GUI (Python)

Connects to the backend socket.

Receives real-time data packets.

Updates a live graph using Matplotlib in a PyQt5 window.

Communication

Uses /tmp/backend_socket for local IPC between backend and GUI.

Messages are sent as formatted text strings (temp,humidity\n).



**Future Enhancements**

Replace simulated I²C with real hardware interface (/dev/i2c-*).

Add control signals (e.g., start/stop from GUI to backend).

Use shared memory IPC for higher performance.

Export logs as CSV or integrate with remote MQTT server.
