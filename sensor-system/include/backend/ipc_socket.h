#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <stdbool.h>
#include "sensor_sim.h"

#define SOCKET_PATH "/tmp/sensor_system.sock"
#define MAX_MSG_SIZE 1024
#define MAX_CLIENTS 10

typedef enum {
    MSG_SENSOR_DATA = 1,
    MSG_SENSOR_LIST,
    MSG_CONTROL,
    MSG_STATUS
} MessageType;

typedef struct {
    MessageType type;
    uint32_t size;
    char data[MAX_MSG_SIZE - sizeof(MessageType) - sizeof(uint32_t)];
} Message;

typedef struct {
    int server_fd;
    int client_fds[MAX_CLIENTS];
    int num_clients;
    bool running;
} SocketServer;

// Server functions
bool socket_server_init(SocketServer *server);
bool socket_server_start(SocketServer *server);
bool socket_server_broadcast(SocketServer *server, const Message *msg);
void socket_server_cleanup(SocketServer *server);

// Client functions
bool socket_send_data(const Sensor sensors[], int count);
bool socket_receive_data(int timeout_ms);

// Message utilities
Message create_sensor_message(const Sensor sensors[], int count);
Message create_status_message(const char *status);

#endif // IPC_SOCKET_H
