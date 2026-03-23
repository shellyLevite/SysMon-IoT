#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "monitor.h"

/* Global definitions (declared extern in monitor.h) */
SystemMetrics          g_metrics       = {0};
pthread_mutex_t        g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t  g_running       = 1;

/* Signal handler — called on Ctrl+C. Just clears the flag; threads exit cleanly. */
static void handle_sigint(int sig)
{
    (void)sig;
    g_running = 0;
}

int main(void)
{
    pthread_t collector_tid, display_tid;

    srand(42); /* Seed for the simulated sensor values */

    /* Register the SIGINT handler (Ctrl+C) for graceful shutdown */
    signal(SIGINT, handle_sigint);

    printf("Starting IoT Device Monitor...\n");

    /* Spawn the two worker threads */
    if (pthread_create(&collector_tid, NULL, data_collector_thread, NULL) != 0) {
        perror("Failed to create collector thread");
        return EXIT_FAILURE;
    }

    if (pthread_create(&display_tid, NULL, data_display_thread, NULL) != 0) {
        perror("Failed to create display thread");
        return EXIT_FAILURE;
    }

    /* Wait for both threads to finish (they exit when g_running becomes 0) */
    pthread_join(collector_tid, NULL);
    pthread_join(display_tid,   NULL);

    pthread_mutex_destroy(&g_metrics_mutex);
    printf("\nIoT monitor stopped cleanly.\n");
    return EXIT_SUCCESS;
}
