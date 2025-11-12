/*
 * backend.c
 * 
 * Reads data from I2C (or simulator), stores to SQLite, and sends to GUI via Unix socket.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>

#define SOCKET_PATH "/tmp/sensor_backend.sock"
#define DB_PATH     "sensor_data.db"
#define I2C_ADDR    0x40
#define POLL_INTERVAL 200000  // microseconds (0.2s)
#define BUF_SIZE 256

// ========== Globals ==========
pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
int server_fd;
int client_fd = -1;
sqlite3 *db = NULL;

typedef struct {
    double temp;
    double hum;
    char ts[64];
} sensor_data_t;

sensor_data_t latest;

// ====== Utilities ======
void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", t);
}

// ====== I2C reading (stub/simulator) ======
int read_i2c_sensor(sensor_data_t *out) {
    // TODO: replace with real I2C read using /dev/i2c-X and ioctl
    // Simulate sensor values
    static double angle = 0;
    angle += 0.1;
    out->temp = 25.0 + 5.0 * sin(angle);
    out->hum  = 40.0 + 10.0 * cos(angle);
    get_timestamp(out->ts, sizeof(out->ts));
    usleep(POLL_INTERVAL);
    return 0;
}

// ====== SQLite ======
void db_init() {
    if (sqlite3_open(DB_PATH, &db)) {
        fprintf(stderr, "Can't open DB: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS readings("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "ts TEXT,temp REAL,hum REAL);";
    char *err = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "DB init error: %s\n", err);
        sqlite3_free(err);
    }
}

void db_insert(sensor_data_t *d) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO readings (ts,temp,hum) VALUES('%s',%.2f,%.2f);",
             d->ts, d->temp, d->hum);
    char *err = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "DB insert error: %s\n", err);
        sqlite3_free(err);
    }
}

// ====== IPC (Unix socket server) ======
void *socket_thread(void *arg) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    chmod(SOCKET_PATH, 0666);
    listen(server_fd, 5);

    printf("[IPC] Waiting for GUI client...\n");
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        return NULL;
    }
    printf("[IPC] GUI connected.\n");

    while (1) {
        pthread_mutex_lock(&data_lock);
        sensor_data_t d = latest;
        pthread_mutex_unlock(&data_lock);

        char msg[256];
        snprintf(msg, sizeof(msg),
                 "{\"ts\":\"%s\",\"temp\":%.2f,\"hum\":%.2f}\n",
                 d.ts, d.temp, d.hum);

        if (write(client_fd, msg, strlen(msg)) < 0) {
            perror("write to client");
            close(client_fd);
            client_fd = -1;
            // Wait for new connection
            client_fd = accept(server_fd, NULL, NULL);
            if (client_fd > 0)
                printf("[IPC] GUI reconnected.\n");
        }
        usleep(POLL_INTERVAL);
    }
    return NULL;
}

// ====== Sensor thread ======
void *sensor_thread(void *arg) {
    while (1) {
        sensor_data_t d;
        read_i2c_sensor(&d);

        pthread_mutex_lock(&data_lock);
        latest = d;
        pthread_mutex_unlock(&data_lock);

        db_insert(&d);
    }
    return NULL;
}

int main() {
    printf("Backend starting...\n");
    db_init();

    pthread_t t1, t2;
    pthread_create(&t1, NULL, sensor_thread, NULL);
    pthread_create(&t2, NULL, socket_thread, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sqlite3_close(db);
    close(server_fd);
    if (client_fd > 0) close(client_fd);
    unlink(SOCKET_PATH);
    return 0;
}

