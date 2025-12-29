"""
Data processing and analysis for sensor data
"""

import numpy as np
from collections import deque
import json
from datetime import datetime

class DataProcessor:
    """Process and analyze sensor data"""
    
    def __init__(self):
        self.history_size = 1000
        self.data_history = {}
        self.statistics = {}
        
    def process_data(self, sensor_data):
        """Process incoming sensor data"""
        processed_data = {}
        
        for sensor_id, sensor in sensor_data.items():
            # Store in history
            if sensor_id not in self.data_history:
                self.data_history[sensor_id] = {
                    'timestamps': deque(maxlen=self.history_size),
                    'temperatures': deque(maxlen=self.history_size),
                    'humidities': deque(maxlen=self.history_size)
                }
            
            # Add current data
            timestamp = datetime.now()
            self.data_history[sensor_id]['timestamps'].append(timestamp)
            self.data_history[sensor_id]['temperatures'].append(sensor['temperature'])
            self.data_history[sensor_id]['humidities'].append(sensor['humidity'])
            
            # Calculate statistics
            self.update_statistics(sensor_id)
            
            # Add processed data
            processed_data[sensor_id] = {
                **sensor,
                'history': {
                    'timestamps': list(self.data_history[sensor_id]['timestamps']),
                    'temperatures': list(self.data_history[sensor_id]['temperatures']),
                    'humidities': list(self.data_history[sensor_id]['humidities'])
                },
                'statistics': self.statistics.get(sensor_id, {})
            }
        
        return processed_data
    
    def update_statistics(self, sensor_id):
        """Update statistics for a sensor"""
        if sensor_id not in self.data_history:
            return
            
        history = self.data_history[sensor_id]
        
        if not history['temperatures']:
            return
            
        # Calculate basic statistics
        temps = list(history['temperatures'])
        hums = list(history['humidities'])
        
        stats = {
            'temp_mean': float(np.mean(temps)),
            'temp_std': float(np.std(temps)),
            'temp_min': float(np.min(temps)),
            'temp_max': float(np.max(temps)),
            'hum_mean': float(np.mean(hums)),
            'hum_std': float(np.std(hums)),
            'hum_min': float(np.min(hums)),
            'hum_max': float(np.max(hums)),
            'sample_count': len(temps)
        }
        
        # Calculate trends (simple linear regression)
        if len(temps) > 1:
            x = np.arange(len(temps))
            temp_trend = np.polyfit(x, temps, 1)[0]
            hum_trend = np.polyfit(x, hums, 1)[0]
            
            stats['temp_trend'] = float(temp_trend)
            stats['hum_trend'] = float(hum_trend)
        
        self.statistics[sensor_id] = stats
    
    def calculate_statistics(self, sensor_data):
        """Calculate statistics for all sensors"""
        all_stats = {}
        
        for sensor_id, sensor in sensor_data.items():
            if 'statistics' in sensor:
                all_stats[sensor_id] = {
                    'name': sensor['name'],
                    **sensor['statistics']
                }
        
        return all_stats
    
    def detect_anomalies(self, sensor_data):
        """Detect anomalies in sensor data"""
        anomalies = []
        
        for sensor_id, sensor in sensor_data.items():
            if sensor_id in self.statistics:
                stats = self.statistics[sensor_id]
                
                # Check for temperature anomalies
                temp = sensor['temperature']
                if abs(temp - stats['temp_mean']) > 3 * stats['temp_std']:
                    anomalies.append({
                        'sensor': sensor['name'],
                        'type': 'temperature',
                        'value': temp,
                        'mean': stats['temp_mean'],
                        'severity': 'high' if abs(temp - stats['temp_mean']) > 5 * stats['temp_std'] else 'medium'
                    })
                
                # Check for humidity anomalies
                hum = sensor['humidity']
                if abs(hum - stats['hum_mean']) > 3 * stats['hum_std']:
                    anomalies.append({
                        'sensor': sensor['name'],
                        'type': 'humidity',
                        'value': hum,
                        'mean': stats['hum_mean'],
                        'severity': 'high' if abs(hum - stats['hum_mean']) > 5 * stats['hum_std'] else 'medium'
                    })
        
        return anomalies
    
    def export_data(self, sensor_id, format='json'):
        """Export sensor data in specified format"""
        if sensor_id not in self.data_history:
            return None
            
        history = self.data_history[sensor_id]
        
        data = {
            'sensor_id': sensor_id,
            'timestamps': [ts.isoformat() for ts in history['timestamps']],
            'temperatures': list(history['temperatures']),
            'humidities': list(history['humidities']),
            'statistics': self.statistics.get(sensor_id, {})
        }
        
        if format.lower() == 'json':
            return json.dumps(data, indent=2, default=str)
        elif format.lower() == 'csv':
            csv_lines = ['timestamp,temperature,humidity']
            for ts, temp, hum in zip(data['timestamps'], data['temperatures'], data['humidities']):
                csv_lines.append(f'{ts},{temp:.2f},{hum:.2f}')
            return '\n'.join(csv_lines)
        
        return None
    
    def clear_history(self, sensor_id=None):
        """Clear history for a sensor or all sensors"""
        if sensor_id:
            if sensor_id in self.data_history:
                del self.data_history[sensor_id]
            if sensor_id in self.statistics:
                del self.statistics[sensor_id]
        else:
            self.data_history.clear()
            self.statistics.clear()
