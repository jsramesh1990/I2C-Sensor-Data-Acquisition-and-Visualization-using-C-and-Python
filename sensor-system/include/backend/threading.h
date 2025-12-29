#ifndef THREADING_H
#define THREADING_H

#include <pthread.h>
#include <stdbool.h>
#include "sensor_sim.h"
#include "database.h"
#include "ipc_socket.h"

typedef struct {
    pthread_t thread;
    bool running;
    int interval_ms;
    void *(*thread_func)(void *);
    void *arg;
} ThreadData;

typedef struct {
    Sensor sensors[MAX_SENSORS];
    Database db;
    SocketServer socket_server;
    ThreadData sensor_thread;
    ThreadData socket_thread;
    ThreadData db_thread;
    pthread_mutex_t data_mutex;
    bool system_running;
} SystemContext;

// Thread management
bool thread_create(ThreadData *thread_data);
bool thread_stop(ThreadData *thread_data);
void thread_cleanup(ThreadData *thread_data);

// System threads
void *sensor_reading_thread(void *arg);
void *socket_server_thread(void *arg);
void *database_thread(void *arg);

// System control
bool system_init(SystemContext *ctx);
void system_run(SystemContext *ctx);
void system_shutdown(SystemContext *ctx);

#endif // THREADING_H
