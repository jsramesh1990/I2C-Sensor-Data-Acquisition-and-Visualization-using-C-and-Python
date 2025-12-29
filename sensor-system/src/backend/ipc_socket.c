#include "ipc_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>

// Initialize socket server
bool socket_server_init(SocketServer *server) {
    if (!server) return false;
    
    // Remove old socket if exists
    unlink(SOCKET_PATH);
    
    // Create socket
    server->server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("socket");
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server->server_fd);
        return false;
    }
    
    // Bind socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->server_fd);
        return false;
    }
    
    // Listen for connections
    if (listen(server->server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server->server_fd);
        return false;
    }
    
    // Initialize client array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->client_fds[i] = -1;
    }
    server->num_clients = 0;
    server->running = true;
    
    printf("Socket server initialized on %s\n", SOCKET_PATH);
    return true;
}

// Start socket server (to be run in a thread)
bool socket_server_start(SocketServer *server) {
    if (!server || !server->running) return false;
    
    printf("Socket server starting...\n");
    
    while (server->running) {
        // Setup poll structure
        struct pollfd fds[MAX_CLIENTS + 1];
        int nfds = 0;
        
        // Add server socket
        fds[nfds].fd = server->server_fd;
        fds[nfds].events = POLLIN;
        nfds++;
        
        // Add client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->client_fds[i] != -1) {
                fds[nfds].fd = server->client_fds[i];
                fds[nfds].events = POLLIN | POLLHUP;
                nfds++;
            }
        }
        
        // Poll with timeout
        int ret = poll(fds, nfds, 100); // 100ms timeout
        
        if (ret < 0) {
            perror("poll");
            break;
        }
        
        if (ret == 0) {
            continue; // Timeout
        }
        
        // Check for new connections
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server->server_fd, NULL, NULL);
            if (client_fd >= 0) {
                // Find empty slot
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (server->client_fds[i] == -1) {
                        slot = i;
                        break;
                    }
                }
                
                if (slot >= 0) {
                    server->client_fds[slot] = client_fd;
                    server->num_clients++;
                    printf("New client connected (fd=%d)\n", client_fd);
                } else {
                    printf("Max clients reached, rejecting connection\n");
                    close(client_fd);
                }
            }
        }
        
        // Check client sockets
        for (int i = 0; i < server->num_clients; i++) {
            if (server->client_fds[i] != -1) {
                // Find the fd in poll structure
                for (int j = 1; j < nfds; j++) {
                    if (fds[j].fd == server->client_fds[i]) {
                        if (fds[j].revents & POLLHUP) {
                            // Client disconnected
                            printf("Client disconnected (fd=%d)\n", server->client_fds[i]);
                            close(server->client_fds[i]);
                            server->client_fds[i] = -1;
                            break;
                        }
                        
                        if (fds[j].revents & POLLIN) {
                            // Handle incoming data
                            Message msg;
                            ssize_t n = recv(server->client_fds[i], &msg, sizeof(msg), 0);
                            
                            if (n > 0) {
                                // Echo back for now
                                send(server->client_fds[i], &msg, n, 0);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    
    return true;
}

// Broadcast message to all clients
bool socket_server_broadcast(SocketServer *server, const Message *msg) {
    if (!server || !msg) return false;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->client_fds[i] != -1) {
            ssize_t sent = send(server->client_fds[i], msg, sizeof(Message), 0);
            if (sent < 0) {
                perror("send");
                // Mark for cleanup
                close(server->client_fds[i]);
                server->client_fds[i] = -1;
            }
        }
    }
    
    return true;
}

// Cleanup socket server
void socket_server_cleanup(SocketServer *server) {
    if (!server) return;
    
    server->running = false;
    
    // Close all client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->client_fds[i] != -1) {
            close(server->client_fds[i]);
            server->client_fds[i] = -1;
        }
    }
    
    // Close server socket
    if (server->server_fd != -1) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    // Remove socket file
    unlink(SOCKET_PATH);
    
    printf("Socket server cleaned up\n");
}

// Send data to socket (client side)
bool socket_send_data(const Sensor sensors[], int count) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return false;
    }
    
    Message msg = create_sensor_message(sensors, count);
    ssize_t sent = send(sock_fd, &msg, sizeof(msg), 0);
    
    close(sock_fd);
    return sent == sizeof(msg);
}

// Receive data from socket (client side)
bool socket_receive_data(int timeout_ms) {
    // Implementation for client-side receiving
    // This would be used if the GUI needs to actively poll
    return true;
}

// Create sensor data message
Message create_sensor_message(const Sensor sensors[], int count) {
    Message msg;
    msg.type = MSG_SENSOR_DATA;
    
    // Serialize sensor data
    int offset = 0;
    for (int i = 0; i < count && offset < sizeof(msg.data) - sizeof(int); i++) {
        // Copy sensor data
        memcpy(msg.data + offset, &sensors[i], sizeof(Sensor));
        offset += sizeof(Sensor);
    }
    
    msg.size = offset;
    return msg;
}

// Create status message
Message create_status_message(const char *status) {
    Message msg;
    msg.type = MSG_STATUS;
    msg.size = strlen(status) + 1;
    strncpy(msg.data, status, sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0';
    return msg;
}
