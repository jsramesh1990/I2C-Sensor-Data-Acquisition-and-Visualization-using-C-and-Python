#!/usr/bin/env python3
"""
Sensor Monitoring System GUI
Main window and application management
"""

import sys
import os
import json
import threading
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QTabWidget, QLabel, QPushButton,
                             QGroupBox, QGridLayout, QComboBox, QSpinBox,
                             QTextEdit, QStatusBar, QMessageBox, QAction,
                             QToolBar, QMenu, QMenuBar, QSplitter)
from PyQt5.QtCore import QTimer, Qt, pyqtSignal, QObject
from PyQt5.QtGui import QIcon, QFont, QPalette, QColor

from plot_widget import PlotWidget
from socket_client import SocketClient
from data_processor import DataProcessor

class SensorGUI(QMainWindow):
    """Main application window"""
    
    update_signal = pyqtSignal(dict)
    
    def __init__(self):
        super().__init__()
        
        self.socket_client = SocketClient()
        self.data_processor = DataProcessor()
        self.sensor_data = {}
        self.is_connected = False
        
        self.init_ui()
        self.init_signals()
        self.init_timers()
        
    def init_ui(self):
        """Initialize the user interface"""
        self.setWindowTitle("Sensor Monitoring System")
        self.setGeometry(100, 100, 1200, 800)
        
        # Set application style
        self.setStyleSheet("""
            QMainWindow {
                background-color: #2b2b2b;
            }
            QLabel {
                color: #ffffff;
                font-size: 12px;
            }
            QPushButton {
                background-color: #3c3c3c;
                color: white;
                border: 1px solid #555;
                border-radius: 4px;
                padding: 5px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #4c4c4c;
                border-color: #666;
            }
            QPushButton:pressed {
                background-color: #2c2c2c;
            }
            QGroupBox {
                color: white;
                border: 2px solid #555;
                border-radius: 5px;
                margin-top: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QComboBox, QSpinBox {
                background-color: #3c3c3c;
                color: white;
                border: 1px solid #555;
                border-radius: 3px;
                padding: 3px;
            }
            QTextEdit {
                background-color: #2c2c2c;
                color: #00ff00;
                font-family: 'Courier New', monospace;
                border: 1px solid #555;
            }
        """)
        
        # Create central widget and main layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Create toolbar
        self.create_toolbar()
        
        # Create main content area
        content_splitter = QSplitter(Qt.Horizontal)
        
        # Left panel - controls and sensor list
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        
        # Connection control group
        conn_group = QGroupBox("Connection Control")
        conn_layout = QGridLayout()
        
        self.conn_status = QLabel("Status: Disconnected")
        self.conn_status.setStyleSheet("color: #ff5555; font-weight: bold;")
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        
        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.clicked.connect(self.refresh_data)
        self.refresh_btn.setEnabled(False)
        
        conn_layout.addWidget(self.conn_status, 0, 0, 1, 2)
        conn_layout.addWidget(self.connect_btn, 1, 0)
        conn_layout.addWidget(self.refresh_btn, 1, 1)
        conn_group.setLayout(conn_layout)
        
        # Sensor selection group
        sensor_group = QGroupBox("Sensor Selection")
        sensor_layout = QVBoxLayout()
        
        self.sensor_combo = QComboBox()
        self.sensor_combo.currentIndexChanged.connect(self.sensor_selected)
        
        self.sensor_info = QTextEdit()
        self.sensor_info.setMaximumHeight(150)
        self.sensor_info.setReadOnly(True)
        
        sensor_layout.addWidget(QLabel("Select Sensor:"))
        sensor_layout.addWidget(self.sensor_combo)
        sensor_layout.addWidget(QLabel("Sensor Info:"))
        sensor_layout.addWidget(self.sensor_info)
        sensor_group.setLayout(sensor_layout)
        
        # Add groups to left panel
        left_layout.addWidget(conn_group)
        left_layout.addWidget(sensor_group)
        left_layout.addStretch()
        
        # Right panel - plots and data
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        
        # Create tab widget for different views
        self.tab_widget = QTabWidget()
        
        # Real-time plot tab
        self.plot_widget = PlotWidget()
        self.tab_widget.addTab(self.plot_widget, "Real-time Plot")
        
        # Statistics tab
        stats_tab = QWidget()
        stats_layout = QVBoxLayout(stats_tab)
        
        self.stats_text = QTextEdit()
        self.stats_text.setReadOnly(True)
        stats_layout.addWidget(self.stats_text)
        
        self.tab_widget.addTab(stats_tab, "Statistics")
        
        # Log tab
        log_tab = QWidget()
        log_layout = QVBoxLayout(log_tab)
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        log_layout.addWidget(self.log_text)
        
        self.tab_widget.addTab(log_tab, "System Log")
        
        right_layout.addWidget(self.tab_widget)
        
        # Add panels to splitter
        content_splitter.addWidget(left_panel)
        content_splitter.addWidget(right_panel)
        content_splitter.setStretchFactor(1, 3)
        
        main_layout.addWidget(content_splitter)
        
        # Create status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")
        
        # Add update time label to status bar
        self.update_time_label = QLabel("Last update: Never")
        self.status_bar.addPermanentWidget(self.update_time_label)
        
    def create_menu_bar(self):
        """Create the application menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("File")
        
        export_action = QAction("Export Data...", self)
        export_action.triggered.connect(self.export_data)
        file_menu.addAction(export_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # View menu
        view_menu = menubar.addMenu("View")
        
        theme_action = QAction("Toggle Theme", self)
        theme_action.triggered.connect(self.toggle_theme)
        view_menu.addAction(theme_action)
        
        # Help menu
        help_menu = menubar.addMenu("Help")
        
        about_action = QAction("About", self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
        
    def create_toolbar(self):
        """Create the application toolbar"""
        toolbar = QToolBar()
        self.addToolBar(toolbar)
        
        self.record_btn = QPushButton("Start Recording")
        self.record_btn.clicked.connect(self.toggle_recording)
        toolbar.addWidget(self.record_btn)
        
        toolbar.addSeparator()
        
        self.clear_btn = QPushButton("Clear Data")
        self.clear_btn.clicked.connect(self.clear_data)
        toolbar.addWidget(self.clear_btn)
        
    def init_signals(self):
        """Initialize signal connections"""
        self.update_signal.connect(self.update_ui)
        self.socket_client.data_received.connect(self.handle_sensor_data)
        self.socket_client.connection_changed.connect(self.handle_connection_change)
        
    def init_timers(self):
        """Initialize timers"""
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self.check_for_updates)
        self.update_timer.start(100)  # Check every 100ms
        
    def toggle_connection(self):
        """Toggle connection to backend"""
        if not self.is_connected:
            if self.socket_client.connect():
                self.log_message("Connected to backend")
            else:
                QMessageBox.warning(self, "Connection Failed",
                                  "Could not connect to backend.\n"
                                  "Make sure the backend is running.")
        else:
            self.socket_client.disconnect()
            self.log_message("Disconnected from backend")
            
    def handle_connection_change(self, connected):
        """Handle connection status changes"""
        self.is_connected = connected
        self.refresh_btn.setEnabled(connected)
        
        if connected:
            self.conn_status.setText("Status: Connected")
            self.conn_status.setStyleSheet("color: #55ff55; font-weight: bold;")
            self.connect_btn.setText("Disconnect")
            self.status_bar.showMessage("Connected to backend")
        else:
            self.conn_status.setText("Status: Disconnected")
            self.conn_status.setStyleSheet("color: #ff5555; font-weight: bold;")
            self.connect_btn.setText("Connect")
            self.status_bar.showMessage("Disconnected")
            
    def handle_sensor_data(self, data):
        """Handle incoming sensor data"""
        try:
            processed_data = self.data_processor.process_data(data)
            self.sensor_data = processed_data
            
            # Update sensor list if needed
            self.update_sensor_list()
            
            # Emit signal for UI update
            self.update_signal.emit(processed_data)
            
        except Exception as e:
            self.log_message(f"Error processing data: {str(e)}")
            
    def update_sensor_list(self):
        """Update the sensor selection combo box"""
        current_selection = self.sensor_combo.currentText()
        self.sensor_combo.clear()
        
        for sensor_id, sensor in self.sensor_data.items():
            display_text = f"{sensor['name']} (0x{sensor['address']:02X})"
            self.sensor_combo.addItem(display_text, sensor_id)
            
        # Try to restore previous selection
        if current_selection:
            index = self.sensor_combo.findText(current_selection)
            if index >= 0:
                self.sensor_combo.setCurrentIndex(index)
                
    def sensor_selected(self, index):
        """Handle sensor selection change"""
        if index >= 0:
            sensor_id = self.sensor_combo.itemData(index)
            if sensor_id in self.sensor_data:
                sensor = self.sensor_data[sensor_id]
                info_text = f"""
                Sensor: {sensor['name']}
                Address: 0x{sensor['address']:02X}
                Temperature: {sensor['temperature']:.2f}°C
                Humidity: {sensor['humidity']:.2f}%
                Status: {'Active' if sensor['active'] else 'Inactive'}
                Last Update: {sensor['timestamp']}
                """
                self.sensor_info.setText(info_text)
                
    def update_ui(self, data):
        """Update the UI with new data"""
        # Update plots
        self.plot_widget.update_data(data)
        
        # Update statistics
        stats = self.data_processor.calculate_statistics(data)
        self.update_statistics_display(stats)
        
        # Update timestamp
        current_time = datetime.now().strftime("%H:%M:%S")
        self.update_time_label.setText(f"Last update: {current_time}")
        
    def update_statistics_display(self, stats):
        """Update the statistics display"""
        stats_text = "=== System Statistics ===\n\n"
        
        for sensor_id, sensor_stats in stats.items():
            stats_text += f"Sensor: {sensor_stats['name']}\n"
            stats_text += f"  Temperature: {sensor_stats['temp_avg']:.2f}°C "
            stats_text += f"(Min: {sensor_stats['temp_min']:.2f}, "
            stats_text += f"Max: {sensor_stats['temp_max']:.2f})\n"
            stats_text += f"  Humidity: {sensor_stats['hum_avg']:.2f}% "
            stats_text += f"(Min: {sensor_stats['hum_min']:.2f}, "
            stats_text += f"Max: {sensor_stats['hum_max']:.2f})\n\n"
            
        self.stats_text.setText(stats_text)
        
    def refresh_data(self):
        """Manually refresh data"""
        if self.is_connected:
            self.log_message("Manual refresh requested")
            # Could implement a request for immediate update
            
    def toggle_recording(self):
        """Toggle data recording"""
        if self.record_btn.text() == "Start Recording":
            self.record_btn.setText("Stop Recording")
            self.log_message("Started recording data")
            # Implement recording logic
        else:
            self.record_btn.setText("Start Recording")
            self.log_message("Stopped recording data")
            
    def clear_data(self):
        """Clear displayed data"""
        reply = QMessageBox.question(self, "Clear Data",
                                   "Clear all displayed data?",
                                   QMessageBox.Yes | QMessageBox.No)
        
        if reply == QMessageBox.Yes:
            self.plot_widget.clear_data()
            self.stats_text.clear()
            self.log_message("Cleared all data")
            
    def export_data(self):
        """Export data to file"""
        # Implement export functionality
        self.log_message("Export functionality not implemented yet")
        
    def toggle_theme(self):
        """Toggle between light and dark themes"""
        # Implement theme switching
        self.log_message("Theme toggle not implemented yet")
        
    def show_about(self):
        """Show about dialog"""
        about_text = """
        Sensor Monitoring System v1.0
        
        A real-time sensor monitoring application
        with backend in C and GUI in Python/PyQt5.
        
        Features:
        • Real-time sensor data visualization
        • Multiple sensor support
        • Historical data storage
        • Statistics and analysis
        
        © 2024 Sensor System Project
        """
        
        QMessageBox.about(self, "About Sensor Monitoring System", about_text)
        
    def log_message(self, message):
        """Add a message to the log"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}"
        self.log_text.append(log_entry)
        
    def check_for_updates(self):
        """Check for updates from socket"""
        # This method is called by timer
        pass
        
    def closeEvent(self, event):
        """Handle window close event"""
        if self.is_connected:
            reply = QMessageBox.question(
                self, "Disconnect",
                "You are still connected to the backend.\n"
                "Do you want to disconnect and exit?",
                QMessageBox.Yes | QMessageBox.No
            )
            
            if reply == QMessageBox.Yes:
                self.socket_client.disconnect()
                event.accept()
            else:
                event.ignore()
        else:
            event.accept()

def main():
    """Main application entry point"""
    app = QApplication(sys.argv)
    
    # Set application icon if available
    # app.setWindowIcon(QIcon('icon.png'))
    
    # Create and show main window
    window = SensorGUI()
    window.show()
    
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
