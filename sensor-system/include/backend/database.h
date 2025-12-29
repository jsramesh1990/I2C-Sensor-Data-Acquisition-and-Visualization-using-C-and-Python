#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdbool.h>
#include "sensor_sim.h"

#define DB_PATH "data/sensor_data.db"
#define MAX_SQL_LENGTH 512

typedef struct {
    sqlite3 *db;
    bool initialized;
} Database;

// Database management
bool db_init(Database *db);
void db_close(Database *db);
bool db_create_tables(Database *db);

// Data operations
bool db_insert_sensor_data(Database *db, const Sensor *sensor, 
                          const char *timestamp);
bool db_get_recent_readings(Database *db, Sensor *sensor, 
                           int max_readings, char ***timestamps, 
                           float **temperatures, float **humidities);
bool db_clear_old_data(Database *db, int days_to_keep);

// Statistics
bool db_get_sensor_stats(Database *db, uint8_t i2c_addr,
                        float *avg_temp, float *avg_hum,
                        float *max_temp, float *min_temp);

#endif // DATABASE_H
