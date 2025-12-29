#ifndef SENSOR_SIM_H
#define SENSOR_SIM_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SENSORS 8
#define I2C_ADDR_BASE 0x40

typedef struct {
    uint8_t i2c_addr;
    float temperature;
    float humidity;
    bool active;
    char name[32];
} Sensor;

typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_NOT_FOUND
} SensorStatus;

// Sensor initialization and management
SensorStatus sensor_init_all(Sensor sensors[], int count);
SensorStatus sensor_read(Sensor *sensor);
SensorStatus sensor_simulate_all(Sensor sensors[], int count);

// Sensor configuration
SensorStatus sensor_set_name(Sensor *sensor, const char *name);
SensorStatus sensor_set_active(Sensor *sensor, bool active);

// Utility functions
float sensor_random_float(float min, float max);
void sensor_print_debug(const Sensor *sensor);

#endif // SENSOR_SIM_H
