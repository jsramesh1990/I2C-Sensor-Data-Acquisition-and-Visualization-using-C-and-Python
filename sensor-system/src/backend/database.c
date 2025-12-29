#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Initialize database connection
bool db_init(Database *db) {
    if (!db) return false;
    
    // Ensure data directory exists
    system("mkdir -p data");
    
    int rc = sqlite3_open(DB_PATH, &db->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db->db));
        db->initialized = false;
        return false;
    }
    
    db->initialized = true;
    
    // Enable foreign keys and WAL mode for better performance
    sqlite3_exec(db->db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    
    return db_create_tables(db);
}

// Close database connection
void db_close(Database *db) {
    if (db && db->initialized) {
        sqlite3_close(db->db);
        db->initialized = false;
    }
}

// Create necessary tables
bool db_create_tables(Database *db) {
    if (!db || !db->initialized) return false;
    
    char *err_msg = NULL;
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS sensors ("
        "sensor_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "i2c_address INTEGER UNIQUE NOT NULL,"
        "name TEXT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "data_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sensor_id INTEGER NOT NULL,"
        "temperature REAL NOT NULL,"
        "humidity REAL NOT NULL,"
        "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (sensor_id) REFERENCES sensors (sensor_id)"
        ");"
        
        "CREATE INDEX IF NOT EXISTS idx_timestamp ON sensor_data(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_sensor_id ON sensor_data(sensor_id);";
    
    int rc = sqlite3_exec(db->db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

// Insert sensor data
bool db_insert_sensor_data(Database *db, const Sensor *sensor, 
                          const char *timestamp) {
    if (!db || !db->initialized || !sensor) return false;
    
    char sql[MAX_SQL_LENGTH];
    sqlite3_stmt *stmt;
    
    // First, ensure sensor exists in sensors table
    const char *check_sensor = 
        "INSERT OR IGNORE INTO sensors (i2c_address, name) VALUES (?, ?);";
    
    if (sqlite3_prepare_v2(db->db, check_sensor, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, sensor->i2c_addr);
    sqlite3_bind_text(stmt, 2, sensor->name, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    // Insert sensor data
    const char *insert_data = 
        "INSERT INTO sensor_data (sensor_id, temperature, humidity, timestamp) "
        "VALUES ((SELECT sensor_id FROM sensors WHERE i2c_address = ?), "
        "?, ?, ?);";
    
    if (sqlite3_prepare_v2(db->db, insert_data, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, sensor->i2c_addr);
    sqlite3_bind_double(stmt, 2, sensor->temperature);
    sqlite3_bind_double(stmt, 3, sensor->humidity);
    
    if (timestamp) {
        sqlite3_bind_text(stmt, 4, timestamp, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// Get recent readings for a sensor
bool db_get_recent_readings(Database *db, Sensor *sensor, 
                           int max_readings, char ***timestamps, 
                           float **temperatures, float **humidities) {
    if (!db || !db->initialized || !sensor || max_readings <= 0) {
        return false;
    }
    
    // Allocate memory for results
    *timestamps = malloc(max_readings * sizeof(char *));
    *temperatures = malloc(max_readings * sizeof(float));
    *humidities = malloc(max_readings * sizeof(float));
    
    if (!*timestamps || !*temperatures || !*humidities) {
        free(*timestamps);
        free(*temperatures);
        free(*humidities);
        return false;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT timestamp, temperature, humidity "
        "FROM sensor_data sd "
        "JOIN sensors s ON sd.sensor_id = s.sensor_id "
        "WHERE s.i2c_address = ? "
        "ORDER BY timestamp DESC LIMIT ?;";
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(*timestamps);
        free(*temperatures);
        free(*humidities);
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, sensor->i2c_addr);
    sqlite3_bind_int(stmt, 2, max_readings);
    
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && row < max_readings) {
        // Copy timestamp
        const char *ts = (const char *)sqlite3_column_text(stmt, 0);
        (*timestamps)[row] = strdup(ts);
        
        // Get numeric values
        (*temperatures)[row] = (float)sqlite3_column_double(stmt, 1);
        (*humidities)[row] = (float)sqlite3_column_double(stmt, 2);
        
        row++;
    }
    
    sqlite3_finalize(stmt);
    return row > 0;
}

// Clear old data
bool db_clear_old_data(Database *db, int days_to_keep) {
    if (!db || !db->initialized || days_to_keep < 0) return false;
    
    char sql[MAX_SQL_LENGTH];
    char *err_msg = NULL;
    
    snprintf(sql, sizeof(sql),
             "DELETE FROM sensor_data "
             "WHERE timestamp < datetime('now', '-%d days');",
             days_to_keep);
    
    int rc = sqlite3_exec(db->db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    // Vacuum to reclaim space
    sqlite3_exec(db->db, "VACUUM;", NULL, NULL, NULL);
    
    return true;
}

// Get sensor statistics
bool db_get_sensor_stats(Database *db, uint8_t i2c_addr,
                        float *avg_temp, float *avg_hum,
                        float *max_temp, float *min_temp) {
    if (!db || !db->initialized) return false;
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT AVG(temperature), AVG(humidity), "
        "MAX(temperature), MIN(temperature) "
        "FROM sensor_data sd "
        "JOIN sensors s ON sd.sensor_id = s.sensor_id "
        "WHERE s.i2c_address = ? "
        "AND timestamp > datetime('now', '-1 day');";
    
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, i2c_addr);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *avg_temp = (float)sqlite3_column_double(stmt, 0);
        *avg_hum = (float)sqlite3_column_double(stmt, 1);
        *max_temp = (float)sqlite3_column_double(stmt, 2);
        *min_temp = (float)sqlite3_column_double(stmt, 3);
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}
