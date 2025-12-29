"""
Plot widget for real-time sensor data visualization
"""

import numpy as np
from PyQt5.QtWidgets import QWidget, QVBoxLayout
from PyQt5.QtCore import Qt, QTimer
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

class PlotWidget(QWidget):
    """Widget for plotting sensor data"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.data_history = {}
        self.max_history = 100  # Keep last 100 points
        
        self.init_ui()
        self.init_plot()
        
    def init_ui(self):
        """Initialize the widget UI"""
        layout = QVBoxLayout(self)
        
        # Create matplotlib figure and canvas
        self.figure = Figure(figsize=(10, 8), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        
        # Create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, self)
        
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)
        
        self.setLayout(layout)
        
    def init_plot(self):
        """Initialize the plot"""
        self.figure.clear()
        
        # Create subplots for temperature and humidity
        self.ax_temp = self.figure.add_subplot(211)
        self.ax_hum = self.figure.add_subplot(212)
        
        # Set titles and labels
        self.ax_temp.set_title('Temperature Over Time')
        self.ax_temp.set_ylabel('Temperature (°C)')
        self.ax_temp.grid(True, alpha=0.3)
        
        self.ax_hum.set_title('Humidity Over Time')
        self.ax_hum.set_xlabel('Time (samples)')
        self.ax_hum.set_ylabel('Humidity (%)')
        self.ax_hum.grid(True, alpha=0.3)
        
        # Create empty line objects for each sensor
        self.temp_lines = {}
        self.hum_lines = {}
        
        self.figure.tight_layout()
        self.canvas.draw()
        
    def update_data(self, sensor_data):
        """Update plot with new sensor data"""
        if not sensor_data:
            return
            
        current_time = len(next(iter(self.data_history.values()))['temperature']) if self.data_history else 0
        
        # Update data history
        for sensor_id, sensor in sensor_data.items():
            if sensor_id not in self.data_history:
                self.data_history[sensor_id] = {
                    'name': sensor['name'],
                    'temperature': [],
                    'humidity': []
                }
            
            # Add new data
            self.data_history[sensor_id]['temperature'].append(sensor['temperature'])
            self.data_history[sensor_id]['humidity'].append(sensor['humidity'])
            
            # Trim history if too long
            if len(self.data_history[sensor_id]['temperature']) > self.max_history:
                self.data_history[sensor_id]['temperature'].pop(0)
                self.data_history[sensor_id]['humidity'].pop(0)
        
        # Update plot
        self.update_plot()
        
    def update_plot(self):
        """Update the plot with current data"""
        # Clear axes
        self.ax_temp.clear()
        self.ax_hum.clear()
        
        # Set titles and labels again
        self.ax_temp.set_title('Temperature Over Time')
        self.ax_temp.set_ylabel('Temperature (°C)')
        self.ax_temp.grid(True, alpha=0.3)
        
        self.ax_hum.set_title('Humidity Over Time')
        self.ax_hum.set_xlabel('Time (samples)')
        self.ax_hum.set_ylabel('Humidity (%)')
        self.ax_hum.grid(True, alpha=0.3)
        
        # Plot data for each sensor
        for sensor_id, data in self.data_history.items():
            if not data['temperature']:
                continue
                
            x_data = list(range(len(data['temperature'])))
            
            # Plot temperature
            line_temp, = self.ax_temp.plot(x_data, data['temperature'], 
                                          label=data['name'], linewidth=2)
            
            # Plot humidity
            line_hum, = self.ax_hum.plot(x_data, data['humidity'],
                                        label=data['name'], linewidth=2)
        
        # Add legends
        if self.data_history:
            self.ax_temp.legend(loc='upper left')
            self.ax_hum.legend(loc='upper left')
            
            # Set y-axis limits
            all_temps = [temp for data in self.data_history.values() 
                        for temp in data['temperature']]
            all_hums = [hum for data in self.data_history.values() 
                       for hum in data['humidity']]
            
            if all_temps:
                temp_min, temp_max = min(all_temps), max(all_temps)
                self.ax_temp.set_ylim(temp_min - 1, temp_max + 1)
                
            if all_hums:
                hum_min, hum_max = min(all_hums), max(all_hums)
                self.ax_hum.set_ylim(max(0, hum_min - 5), min(100, hum_max + 5))
        
        self.figure.tight_layout()
        self.canvas.draw()
        
    def clear_data(self):
        """Clear all plot data"""
        self.data_history.clear()
        self.init_plot()
        
    def save_plot(self, filename):
        """Save the current plot to a file"""
        if filename:
            self.figure.savefig(filename, dpi=300, bbox_inches='tight')
            return True
        return False
