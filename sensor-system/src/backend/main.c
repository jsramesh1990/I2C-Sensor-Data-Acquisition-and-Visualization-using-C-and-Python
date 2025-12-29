#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "threading.h"

static SystemContext ctx;
static volatile sig_atomic_t keep_running = 1;

void signal_handler(int signum) {
    printf("\nReceived signal %d, shutting down...\n", signum);
    keep_running = 0;
}

void setup_signals() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe signals
}

int main(int argc, char *argv[]) {
    printf("=== Sensor Monitoring System Backend ===\n");
    
    // Setup signal handling
    setup_signals();
    
    // Initialize system context
    if (!system_init(&ctx)) {
        fprintf(stderr, "Failed to initialize system\n");
        return EXIT_FAILURE;
    }
    
    printf("System initialized. Starting threads...\n");
    
    // Create threads
    if (!thread_create(&ctx.sensor_thread) ||
        !thread_create(&ctx.socket_thread) ||
        !thread_create(&ctx.db_thread)) {
        fprintf(stderr, "Failed to create threads\n");
        system_shutdown(&ctx);
        return EXIT_FAILURE;
    }
    
    printf("All threads started. System running.\n");
    printf("Press Ctrl+C to stop.\n");
    
    // Main loop
    while (keep_running) {
        sleep(1);
        
        // Periodically print status
        static int counter = 0;
        if (++counter % 10 == 0) {
            printf("System running... (Press Ctrl+C to stop)\n");
        }
    }
    
    // Shutdown
    printf("\nInitiating shutdown...\n");
    system_shutdown(&ctx);
    
    printf("Cleanup complete. Goodbye!\n");
    return EXIT_SUCCESS;
}
