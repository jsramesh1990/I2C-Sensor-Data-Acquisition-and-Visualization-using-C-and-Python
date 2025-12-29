"""
Unix socket client for communicating with the backend
"""

import socket
import json
import struct
from threading import Thread
from PyQt5.QtCore import QObject, pyqtSignal, QTimer
import time

class SocketClient(QObject):
    """Client for Unix socket communication"""
    
    data_received = pyqtSignal(dict)
    connection_changed = pyqtSignal(bool)
    
    def __init__(self):
        super().__init__()
        
        self.socket_path = "/tmp/sensor_system.sock"
        self.sock = None
        self.connected = False
        self.receive_thread = None
        self.receiving = False
        
    def connect(self):
        """Connect to the Unix socket server"""
        try:
            # Create socket
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            
            # Connect to server
            self.sock.connect(self.socket_path)
            
            self.connected = True
            self.connection_changed.emit(True)
            
            # Start receive thread
            self.receiving = True
            self.receive_thread = Thread(target=self.receive_loop, daemon=True)
            self.receive_thread.start()
            
            return True
            
        except Exception as e:
            print(f"Connection error: {e}")
            self.connected = False
            self.connection_changed.emit(False)
            return False
    
    def disconnect(self):
        """Disconnect from the server"""
        self.receiving = False
        
        if self.receive_thread:
            self.receive_thread.join(timeout=1.0)
            self.receive_thread = None
            
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.sock = None
            
        self.connected = False
        self.connection_changed.emit(False)
    
    def receive_loop(self):
        """Main receive loop (runs in thread)"""
        while self.receiving and self.connected:
            try:
                # Receive message header (type + size)
                header = self.sock.recv(8)  # 4 bytes for type + 4 bytes for size
                if len(header) < 8:
                    if len(header) == 0:
                        # Connection closed
                        break
                    continue
                
                # Parse header
                msg_type, msg_size = struct.unpack('II', header)
                
                # Receive data
                data = b''
                while len(data) < msg_size:
                    chunk = self.sock.recv(min(4096, msg_size - len(data)))
                    if not chunk:
                        break
                    data += chunk
                
                if len(data) == msg_size:
                    self.process_message(msg_type, data)
                    
            except ConnectionError:
                break
            except Exception as e:
                print(f"Receive error: {e}")
                time.sleep(0.1)
        
        # Connection lost
        self.connected = False
        self.connection_changed.emit(False)
    
    def process_message(self, msg_type, data):
        """Process received message"""
        try:
            if msg_type == 1:  # MSG_SENSOR_DATA
                # Parse sensor data
                sensors = []
                
                # Each sensor is 32 + 4 + 4 + 1 + 32 = 73 bytes (with padding)
                sensor_size = 73
                num_sensors = len(data) // sensor_size
                
                for i in range(num_sensors):
                    offset = i * sensor_size
                    
                    # Extract sensor data
                    # Note: This assumes the C struct layout
                    # In practice, you'd need proper serialization/deserialization
                    i2c_addr = data[offset]
                    temperature = struct.unpack('f', data[offset+32:offset+36])[0]
                    humidity = struct.unpack('f', data[offset+36:offset+40])[0]
                    active = bool(data[offset+40])
                    name = data[offset+41:offset+73].split(b'\x00')[0].decode('utf-8')
                    
                    sensors.append({
                        'address': i2c_addr,
                        'name': name,
                        'temperature': temperature,
                        'humidity': humidity,
                        'active': active
                    })
                
                # Convert to dictionary
                sensor_dict = {}
                for sensor in sensors:
                    sensor_id = f"sensor_{sensor['address']:02x}"
                    sensor_dict[sensor_id] = {
                        'id': sensor_id,
                        'address': sensor['address'],
                        'name': sensor['name'],
                        'temperature': sensor['temperature'],
                        'humidity': sensor['humidity'],
                        'active': sensor['active'],
                        'timestamp': time.strftime("%H:%M:%S")
                    }
                
                # Emit signal with data
                self.data_received.emit(sensor_dict)
                
            elif msg_type == 4:  # MSG_STATUS
                status = data.decode('utf-8').rstrip('\x00')
                print(f"Status: {status}")
                
        except Exception as e:
            print(f"Error processing message: {e}")
    
    def send_message(self, msg_type, data):
        """Send a message to the server"""
        if not self.connected:
            return False
            
        try:
            # Create message
            msg_size = len(data)
            header = struct.pack('II', msg_type, msg_size)
            
            # Send
            self.sock.sendall(header + data)
            return True
            
        except Exception as e:
            print(f"Send error: {e}")
            self.disconnect()
            return False
    
    def is_connected(self):
        """Check if connected"""
        return self.connected
