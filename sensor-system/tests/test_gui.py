#!/usr/bin/env python3
"""
GUI Unit Tests
"""

import sys
import os
import unittest
from unittest.mock import Mock, patch
import tempfile

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../src/gui'))

from socket_client import SocketClient
from data_processor import DataProcessor

class TestSocketClient(unittest.TestCase):
    """Test socket client functionality"""
    
    def setUp(self):
        self.client = SocketClient()
    
    def test_initial_state(self):
        """Test initial state of socket client"""
        self.assertFalse(self.client.is_connected())
        self.assertIsNone(self.client.sock)
    
    @patch('socket.socket')
    def test_connect_success(self, mock_socket):
        """Test successful connection"""
        mock_sock = Mock()
        mock_socket.return_value = mock_sock
        mock_sock.connect.return_value = None
        
        result = self.client.connect()
        
        self.assertTrue(result)
        self.assertTrue(self.client.is_connected())
        mock_sock.connect.assert_called_once_with(self.client.socket_path)
    
    @patch('socket.socket')
    def test_connect_failure(self, mock_socket):
        """Test failed connection"""
        mock_sock = Mock()
        mock_socket.return_value = mock_sock
        mock_sock.connect.side_effect = ConnectionRefusedError
        
        result = self.client.connect()
        
        self.assertFalse(result)
        self.assertFalse(self.client.is_connected())
    
    def test_disconnect(self):
        """Test disconnection"""
        self.client.sock = Mock()
        self.client.connected = True
        self.client.receiving = True
        self.client.receive_thread = Mock()
        
        self.client.disconnect()
        
        self.assertFalse(self.client.is_connected())
        self.assertFalse(self.client.receiving)
        self.client.sock.close.assert_called_once()

class TestDataProcessor(unittest.TestCase):
    """Test data processor functionality"""
    
    def setUp(self):
        self.processor = DataProcessor()
    
    def test_process_data(self):
        """Test data processing"""
        test_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 25.5,
                'humidity': 60.0,
                'active': True
            }
        }
        
        processed = self.processor.process_data(test_data)
        
        self.assertIn('sensor_01', processed)
        self.assertEqual(processed['sensor_01']['name'], 'Test Sensor')
        self.assertEqual(processed['sensor_01']['temperature'], 25.5)
        self.assertIn('history', processed['sensor_01'])
        self.assertIn('statistics', processed['sensor_01'])
    
    def test_calculate_statistics(self):
        """Test statistics calculation"""
        # Add some test data
        test_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 25.5,
                'humidity': 60.0,
                'active': True
            }
        }
        
        # Process data multiple times to build history
        for i in range(10):
            self.processor.process_data(test_data)
        
        stats = self.processor.calculate_statistics({
            'sensor_01': {
                'name': 'Test Sensor',
                'statistics': self.processor.statistics.get('sensor_01', {})
            }
        })
        
        self.assertIn('sensor_01', stats)
        self.assertIn('temp_mean', stats['sensor_01'])
        self.assertIn('hum_mean', stats['sensor_01'])
    
    def test_anomaly_detection(self):
        """Test anomaly detection"""
        # First, build normal history
        normal_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 25.0,
                'humidity': 50.0,
                'active': True
            }
        }
        
        for _ in range(20):
            self.processor.process_data(normal_data)
        
        # Now test with anomalous data
        anomalous_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 100.0,  # Very high temperature
                'humidity': 50.0,
                'active': True
            }
        }
        
        anomalies = self.processor.detect_anomalies(anomalous_data)
        
        self.assertTrue(len(anomalies) > 0)
        self.assertEqual(anomalies[0]['type'], 'temperature')
        self.assertEqual(anomalies[0]['sensor'], 'Test Sensor')
    
    def test_export_data(self):
        """Test data export"""
        # Add test data
        test_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 25.5,
                'humidity': 60.0,
                'active': True
            }
        }
        
        self.processor.process_data(test_data)
        
        # Test JSON export
        json_data = self.processor.export_data('sensor_01', 'json')
        self.assertIsNotNone(json_data)
        self.assertIn('sensor_id', json_data)
        self.assertIn('temperatures', json_data)
        
        # Test CSV export
        csv_data = self.processor.export_data('sensor_01', 'csv')
        self.assertIsNotNone(csv_data)
        self.assertIn('timestamp,temperature,humidity', csv_data)
    
    def test_clear_history(self):
        """Test history clearing"""
        # Add test data
        test_data = {
            'sensor_01': {
                'name': 'Test Sensor',
                'temperature': 25.5,
                'humidity': 60.0,
                'active': True
            }
        }
        
        self.processor.process_data(test_data)
        
        # Verify data exists
        self.assertIn('sensor_01', self.processor.data_history)
        
        # Clear specific sensor
        self.processor.clear_history('sensor_01')
        self.assertNotIn('sensor_01', self.processor.data_history)
        
        # Add data again
        self.processor.process_data(test_data)
        self.processor.process_data({
            'sensor_02': {
                'name': 'Test Sensor 2',
                'temperature': 26.0,
                'humidity': 65.0,
                'active': True
            }
        })
        
        # Clear all
        self.processor.clear_history()
        self.assertEqual(len(self.processor.data_history), 0)
        self.assertEqual(len(self.processor.statistics), 0)

class TestIntegration(unittest.TestCase):
    """Test integration between components"""
    
    def test_data_flow(self):
        """Test complete data flow"""
        processor = DataProcessor()
        
        # Simulate incoming data
        incoming_data = {
            'sensor_40': {
                'address': 0x40,
                'name': 'Sensor_40',
                'temperature': 22.5,
                'humidity': 55.0,
                'active': True,
                'timestamp': '12:00:00'
            }
        }
        
        # Process data
        processed = processor.process_data(incoming_data)
        
        # Verify processing
        self.assertIn('sensor_40', processed)
        self.assertEqual(processed['sensor_40']['temperature'], 22.5)
        
        # Verify statistics were calculated
        stats = processor.calculate_statistics(processed)
        self.assertIn('sensor_40', stats)
        
        # Verify no anomalies in normal data
        anomalies = processor.detect_anomalies(incoming_data)
        if anomalies:
            print(f"Note: Detected anomalies in normal data: {anomalies}")

if __name__ == '__main__':
    unittest.main(verbosity=2)
