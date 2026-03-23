#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <signal.h>

/* Holds all system metrics shared between threads */
typedef struct {
    float  cpu_usage;          /* CPU usage percentage           */
    long   free_memory;        /* Free memory in MB              */
    float  temperature;        /* CPU temperature in Celsius     */
    int    simulated_sensor;   /* Simulated IoT sensor value     */
} SystemMetrics;

/* Global shared data and mutex — defined in main.c */
extern SystemMetrics g_metrics;
extern pthread_mutex_t g_metrics_mutex;

/* Global running flag — set to 0 by the SIGINT handler to stop all threads */
extern volatile sig_atomic_t g_running;

/* Thread function declarations */
void *data_collector_thread(void *arg);
void *data_display_thread(void *arg);

#endif /* MONITOR_H */
