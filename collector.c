#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "monitor.h"

#define LOG_FILE "iot_monitor.csv"

/* -------------------------------------------------------------------
 * read_cpu_usage()
 * Reads two snapshots of /proc/stat 100 ms apart and calculates the
 * percentage of time the CPU was NOT idle between them.
 * ------------------------------------------------------------------- */
static float read_cpu_usage(void)
{
    long user1, nice1, sys1, idle1;
    long user2, nice2, sys2, idle2;
    char label[16];
    FILE *fp;

    fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0f;
    fscanf(fp, "%s %ld %ld %ld %ld", label, &user1, &nice1, &sys1, &idle1);
    fclose(fp);

    usleep(100000); /* 100 ms between samples */

    fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0f;
    fscanf(fp, "%s %ld %ld %ld %ld", label, &user2, &nice2, &sys2, &idle2);
    fclose(fp);

    long total_delta = (user2 + nice2 + sys2 + idle2) - (user1 + nice1 + sys1 + idle1);
    long idle_delta  = idle2 - idle1;

    if (total_delta == 0) return 0.0f;
    return (float)(total_delta - idle_delta) / (float)total_delta * 100.0f;
}

/* -------------------------------------------------------------------
 * read_free_memory_mb()
 * Parses "MemFree:" from /proc/meminfo and converts kB → MB.
 * ------------------------------------------------------------------- */
static long read_free_memory_mb(void)
{
    char line[128];
    long free_kb = 0;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line, "MemFree: %ld kB", &free_kb);
            break;
        }
    }
    fclose(fp);
    return free_kb / 1024;
}

/* -------------------------------------------------------------------
 * read_temperature()
 * Reads the raw millidegree value from the thermal sysfs node.
 * Returns 0.0 gracefully if the file does not exist (e.g., on WSL).
 * ------------------------------------------------------------------- */
static float read_temperature(void)
{
    int raw_temp = 0;
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return 0.0f; /* Not available on this system */

    fscanf(fp, "%d", &raw_temp);
    fclose(fp);
    return (float)raw_temp / 1000.0f; /* Convert millidegrees → Celsius */
}

/* -------------------------------------------------------------------
 * data_collector_thread()
 * Runs in a loop every second.  Collects all metrics, then locks the
 * shared struct only long enough to copy the fresh values in.
 * ------------------------------------------------------------------- */
void *data_collector_thread(void *arg)
{
    (void)arg; /* unused parameter */

    /* Open the CSV log file once for the lifetime of the thread */
    FILE *logfile = fopen(LOG_FILE, "w");
    if (logfile) {
        fprintf(logfile, "timestamp,cpu_usage,free_memory_mb,temperature_c,sensor\n");
        fflush(logfile);
    }

    while (g_running) {
        /* Collect data OUTSIDE the lock so we don't block the display */
        float cpu   = read_cpu_usage();
        long  mem   = read_free_memory_mb();
        float temp  = read_temperature();
        int   sensor = (rand() % 100) + 1; /* Simulated IoT sensor: 1–100 */

        /* Lock only to write the result into the shared struct */
        pthread_mutex_lock(&g_metrics_mutex);
        g_metrics.cpu_usage        = cpu;
        g_metrics.free_memory      = mem;
        g_metrics.temperature      = temp;
        g_metrics.simulated_sensor = sensor;
        pthread_mutex_unlock(&g_metrics_mutex);

        /* Append one CSV row — done outside the lock, uses local copies */
        if (logfile) {
            time_t now = time(NULL);
            char   ts[20];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
            fprintf(logfile, "%s,%.1f,%ld,%.1f,%d\n",
                    ts, cpu, mem, temp, sensor);
            fflush(logfile);
        }

        sleep(1);
    }

    if (logfile)
        fclose(logfile);

    return NULL; /* Reached when g_running == 0 */
}
