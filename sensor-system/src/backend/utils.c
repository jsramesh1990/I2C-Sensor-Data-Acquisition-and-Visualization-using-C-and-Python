#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sensor_sim.h"

// Print current timestamp
void print_timestamp() {
    time_t now = time(NULL);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] ", buf);
}

// Convert sensor data to JSON string
char* sensor_to_json(const Sensor *sensor) {
    if (!sensor) return NULL;
    
    char *json = malloc(256);
    if (!json) return NULL;
    
    snprintf(json, 256,
             "{\"address\":\"0x%02X\",\"name\":\"%s\","
             "\"temperature\":%.2f,\"humidity\":%.2f,"
             "\"active\":%s}",
             sensor->i2c_addr, sensor->name,
             sensor->temperature, sensor->humidity,
             sensor->active ? "true" : "false");
    
    return json;
}

// Parse command line arguments
void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 ||
            strcmp(argv[i], "-v") == 0) {
            // Enable verbose logging
            printf("Verbose mode enabled\n");
        } else if (strcmp(argv[i], "--help") == 0 ||
                  strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -v, --verbose    Enable verbose logging\n");
            printf("  -h, --help       Show this help message\n");
            exit(0);
        }
    }
}

// Calculate statistics
void calculate_stats(const float *data, int count, 
                    float *min, float *max, float *avg) {
    if (!data || count <= 0 || !min || !max || !avg) return;
    
    *min = data[0];
    *max = data[0];
    float sum = 0;
    
    for (int i = 0; i < count; i++) {
        if (data[i] < *min) *min = data[i];
        if (data[i] > *max) *max = data[i];
        sum += data[i];
    }
    
    *avg = sum / count;
}

// Safe string copy
void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}
