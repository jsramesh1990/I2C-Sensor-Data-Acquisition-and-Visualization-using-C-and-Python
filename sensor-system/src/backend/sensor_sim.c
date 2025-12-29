#include "sensor_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

// Initialize all sensors
SensorStatus sensor_init_all(Sensor sensors[], int count) {
    if (count > MAX_SENSORS) {
        return SENSOR_ERROR;
    }
    
    srand(time(NULL) ^ getpid());
    
    for (int i = 0; i < count; i++) {
        sensors[i].i2c_addr = I2C_ADDR_BASE + i;
        sensors[i].temperature = 20.0 + (rand() % 100) / 10.0;
        sensors[i].humidity = 40.0 + (rand() % 400) / 10.0;
        sensors[i].active = true;
        snprintf(sensors[i].name, sizeof(sensors[i].name), 
                "Sensor_%02X", sensors[i].i2c_addr);
    }
    
    return SENSOR_OK;
}

// Read simulated sensor data
SensorStatus sensor_read(Sensor *sensor) {
    if (!sensor || !sensor->active) {
        return SENSOR_NOT_FOUND;
    }
    
    // Add some randomness with a trend
    float temp_change = sensor_random_float(-0.5, 0.5);
    float hum_change = sensor_random_float(-1.0, 1.0);
    
    // Apply changes with limits
    sensor->temperature += temp_change;
    sensor->humidity += hum_change;
    
    // Clamp values to realistic ranges
    if (sensor->temperature < -10.0) sensor->temperature = -10.0;
    if (sensor->temperature > 50.0) sensor->temperature = 50.0;
    if (sensor->humidity < 0.0) sensor->humidity = 0.0;
    if (sensor->humidity > 100.0) sensor->humidity = 100.0;
    
    // Add some periodic variation
    time_t now = time(NULL);
    sensor->temperature += sin(now / 10.0) * 0.1;
    sensor->humidity += cos(now / 15.0) * 0.5;
    
    return SENSOR_OK;
}

// Simulate reading all sensors
SensorStatus sensor_simulate_all(Sensor sensors[], int count) {
    SensorStatus overall_status = SENSOR_OK;
    
    for (int i = 0; i < count; i++) {
        if (sensors[i].active) {
            SensorStatus status = sensor_read(&sensors[i]);
            if (status != SENSOR_OK) {
                overall_status = status;
            }
        }
    }
    
    return overall_status;
}

// Set sensor name
SensorStatus sensor_set_name(Sensor *sensor, const char *name) {
    if (!sensor || !name) {
        return SENSOR_ERROR;
    }
    
    strncpy(sensor->name, name, sizeof(sensor->name) - 1);
    sensor->name[sizeof(sensor->name) - 1] = '\0';
    return SENSOR_OK;
}

// Set sensor active state
SensorStatus sensor_set_active(Sensor *sensor, bool active) {
    if (!sensor) {
        return SENSOR_ERROR;
    }
    
    sensor->active = active;
    return SENSOR_OK;
}

// Generate random float between min and max
float sensor_random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// Print sensor debug info
void sensor_print_debug(const Sensor *sensor) {
    if (!sensor) {
        printf("Sensor: NULL\n");
        return;
    }
    
    printf("Sensor %s (0x%02X):\n", sensor->name, sensor->i2c_addr);
    printf("  Temperature: %.2f°C\n", sensor->temperature);
    printf("  Humidity: %.2f%%\n", sensor->humidity);
    printf("  Status: %s\n", sensor->active ? "Active" : "Inactive");
}
