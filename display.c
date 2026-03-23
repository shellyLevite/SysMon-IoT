#include <stdio.h>
#include <unistd.h>
#include "monitor.h"

/* -------------------------------------------------------------------
 * data_display_thread()
 * Wakes up every second, grabs a snapshot of the current metrics
 * (holding the mutex only briefly), then renders a dashboard.
 * ------------------------------------------------------------------- */
void *data_display_thread(void *arg)
{
    (void)arg; /* unused parameter */

    while (1) {
        /* Take a local copy so we hold the mutex as briefly as possible */
        SystemMetrics snapshot;
        pthread_mutex_lock(&g_metrics_mutex);
        snapshot = g_metrics;
        pthread_mutex_unlock(&g_metrics_mutex);

        /* Clear screen and move cursor to top-left */
        printf("\033[H\033[J");

        printf("=========================================\n");
        printf("   Embedded IoT Device Monitor — Linux  \n");
        printf("=========================================\n");

        printf("  CPU Usage       : %.1f %%\n",  snapshot.cpu_usage);
        printf("  Free Memory     : %ld MB\n",   snapshot.free_memory);

        if (snapshot.temperature > 0.0f)
            printf("  Temperature     : %.1f °C\n", snapshot.temperature);
        else
            printf("  Temperature     : N/A (not available on this system)\n");

        printf("  IoT Sensor      : %d units\n", snapshot.simulated_sensor);
        printf("-----------------------------------------\n");
        printf("  Press Ctrl+C to exit.\n");
        printf("=========================================\n");

        fflush(stdout);
        sleep(1);
    }
    return NULL;
}
