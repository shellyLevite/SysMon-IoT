#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "monitor.h"

/* Global definitions (declared extern in monitor.h) */
SystemMetrics   g_metrics       = {0};
pthread_mutex_t g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    pthread_t collector_tid, display_tid;

    srand(42); /* Seed for the simulated sensor values */

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

    /* Wait forever — the threads loop indefinitely until Ctrl+C */
    pthread_join(collector_tid, NULL);
    pthread_join(display_tid,   NULL);

    pthread_mutex_destroy(&g_metrics_mutex);
    return EXIT_SUCCESS;
}
