#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Create a new thread
bool thread_create(ThreadData *thread_data) {
    if (!thread_data || !thread_data->thread_func) {
        return false;
    }
    
    thread_data->running = true;
    
    int result = pthread_create(&thread_data->thread, NULL, 
                               thread_data->thread_func, thread_data->arg);
    if (result != 0) {
        fprintf(stderr, "Failed to create thread: %d\n", result);
        thread_data->running = false;
        return false;
    }
    
    return true;
}

// Stop a running thread
bool thread_stop(ThreadData *thread_data) {
    if (!thread_data) return false;
    
    thread_data->running = false;
    void *retval;
    
    if (pthread_join(thread_data->thread, &retval) != 0) {
        fprintf(stderr, "Failed to join thread\n");
        return false;
    }
    
    return true;
}

// Cleanup thread resources
void thread_cleanup(ThreadData *thread_data) {
    if (!thread_data) return;
    
    if (thread_data->running) {
        thread_stop(thread_data);
    }
    
    memset(thread_data, 0, sizeof(ThreadData));
}

// Sensor reading thread function
void *sensor_reading_thread(void *arg) {
    SystemContext *ctx = (SystemContext *)arg;
    if (!ctx) return NULL;
    
    printf("Sensor thread started\n");
    
    while (ctx->sensor_thread.running) {
        // Lock mutex for thread-safe sensor access
        pthread_mutex_lock(&ctx->data_mutex);
        
        // Read all sensors
        sensor_simulate_all(ctx->sensors, MAX_SENSORS);
        
        // Create timestamp
        time_t now = time(NULL);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", 
                localtime(&now));
        
        // Insert into database
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (ctx->sensors[i].active) {
                db_insert_sensor_data(&ctx->db, &ctx->sensors[i], timestamp);
            }
        }
        
        // Create and send message
        Message msg = create_sensor_message(ctx->sensors, MAX_SENSORS);
        socket_server_broadcast(&ctx->socket_server, &msg);
        
        pthread_mutex_unlock(&ctx->data_mutex);
        
        // Sleep for interval
        usleep(ctx->sensor_thread.interval_ms * 1000);
    }
    
    printf("Sensor thread stopped\n");
    return NULL;
}

// Socket server thread function
void *socket_server_thread(void *arg) {
    SystemContext *ctx = (SystemContext *)arg;
    if (!ctx) return NULL;
    
    printf("Socket thread started\n");
    
    // Run socket server
    socket_server_start(&ctx->socket_server);
    
    printf("Socket thread stopped\n");
    return NULL;
}

// Database maintenance thread function
void *database_thread(void *arg) {
    SystemContext *ctx = (SystemContext *)arg;
    if (!ctx) return NULL;
    
    printf("Database thread started\n");
    
    int counter = 0;
    while (ctx->db_thread.running) {
        // Every minute, do database maintenance
        if (++counter >= 60) { // Assuming 1 second sleep
            pthread_mutex_lock(&ctx->data_mutex);
            
            // Clear data older than 7 days
            db_clear_old_data(&ctx->db, 7);
            
            pthread_mutex_unlock(&ctx->data_mutex);
            counter = 0;
        }
        
        sleep(1);
    }
    
    printf("Database thread stopped\n");
    return NULL;
}

// Initialize system context
bool system_init(SystemContext *ctx) {
    if (!ctx) return false;
    
    memset(ctx, 0, sizeof(SystemContext));
    
    // Initialize mutex
    if (pthread_mutex_init(&ctx->data_mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        return false;
    }
    
    // Initialize sensors
    if (sensor_init_all(ctx->sensors, MAX_SENSORS) != SENSOR_OK) {
        fprintf(stderr, "Failed to initialize sensors\n");
        return false;
    }
    
    // Initialize database
    if (!db_init(&ctx->db)) {
        fprintf(stderr, "Failed to initialize database\n");
        return false;
    }
    
    // Initialize socket server
    if (!socket_server_init(&ctx->socket_server)) {
        fprintf(stderr, "Failed to initialize socket server\n");
        db_close(&ctx->db);
        return false;
    }
    
    // Setup sensor thread
    ctx->sensor_thread.thread_func = sensor_reading_thread;
    ctx->sensor_thread.arg = ctx;
    ctx->sensor_thread.interval_ms = 1000; // 1 second
    
    // Setup socket thread
    ctx->socket_thread.thread_func = socket_server_thread;
    ctx->socket_thread.arg = ctx;
    ctx->socket_thread.interval_ms = 0;
    
    // Setup database thread
    ctx->db_thread.thread_func = database_thread;
    ctx->db_thread.arg = ctx;
    ctx->db_thread.interval_ms = 0;
    
    ctx->system_running = true;
    
    printf("System context initialized successfully\n");
    return true;
}

// Run system (main loop)
void system_run(SystemContext *ctx) {
    if (!ctx) return;
    
    // This function is called from main.c
    // The actual thread management happens in main.c
}

// Shutdown system
void system_shutdown(SystemContext *ctx) {
    if (!ctx) return;
    
    printf("Shutting down system...\n");
    ctx->system_running = false;
    
    // Stop all threads
    thread_stop(&ctx->sensor_thread);
    thread_stop(&ctx->socket_thread);
    thread_stop(&ctx->db_thread);
    
    // Cleanup resources
    socket_server_cleanup(&ctx->socket_server);
    db_close(&ctx->db);
    
    // Destroy mutex
    pthread_mutex_destroy(&ctx->data_mutex);
    
    printf("System shutdown complete\n");
}
