# SysMon-IoT — Embedded IoT Device Monitor

A lightweight, real-time system monitor written in C for Linux.  
Collects CPU, memory, and temperature metrics from `/proc` and `/sys`, and simulates an IoT sensor feed — all running concurrently with POSIX threads.

```
=========================================
   Embedded IoT Device Monitor — Linux
=========================================
  CPU Usage       : 12.4 %
  Free Memory     : 1823 MB
  Temperature     : 47.0 °C
  IoT Sensor      : 63 units
-----------------------------------------
  Press Ctrl+C to exit.
=========================================
```

---

## Build & Run

```bash
sudo apt install gcc make   # install if needed
make
./iot_monitor
```

Press **Ctrl+C** to exit.

---

## Project Structure

```
projectnew/
├── monitor.h      # Phase 1 — Shared struct & declarations
├── collector.c    # Phase 2 — Data collection thread
├── display.c      # Phase 3 — Real-time display thread
├── main.c         # Phase 4 — Entry point & thread management
└── Makefile       # Phase 4 — Build system
```

---

## Phase 1 — `monitor.h` · Shared Types & Declarations

This header is the contract between all modules.

```c
typedef struct {
    float  cpu_usage;
    long   free_memory;
    float  temperature;
    int    simulated_sensor;
} SystemMetrics;
```

- **`SystemMetrics`** — one struct that holds every metric. Both threads access this, so it lives in shared memory.
- **`extern` globals** — `g_metrics` and `g_metrics_mutex` are declared here but *defined* in `main.c`. This is the standard C pattern to share data across multiple translation units without duplicate definitions.
- **Thread function declarations** — lets the compiler type-check the function signatures before the implementations are seen.

---

## Phase 2 — `collector.c` · Data Collection Thread

Runs `data_collector_thread()` in an infinite loop, waking every second.

### CPU Usage — `/proc/stat`
```
cpu  4705 356 584 3699 23 0 0 0 0 0
      user nice sys  idle ...
```
The kernel exposes cumulative CPU ticks. The trick is to read it **twice** with a 100 ms gap and calculate:

$$\text{CPU\%} = \frac{(\text{total}_2 - \text{total}_1) - (\text{idle}_2 - \text{idle}_1)}{\text{total}_2 - \text{total}_1} \times 100$$

### Free Memory — `/proc/meminfo`
Scans line by line with `fgets` until it finds `MemFree:`, then converts kB → MB. Simple, zero dependencies.

### Temperature — `/sys/class/thermal/thermal_zone0/temp`
The kernel stores temperature as an integer in **millidegrees Celsius** (e.g., `47000` = 47.0 °C).  
If the file doesn't exist (WSL, VMs, some containers) the function returns `0.0` gracefully — no crash.

### Simulated IoT Sensor
```c
int sensor = (rand() % 100) + 1;
```
A random integer 1–100 simulating a real-world sensor (e.g., humidity, light, vibration).

### Thread Safety Pattern
The heavy I/O happens **outside** the mutex. The lock is held only for the brief struct write at the end. This keeps the display thread from ever waiting long.

---

## Phase 3 — `display.c` · Real-Time Display Thread

Runs `data_display_thread()` in an infinite loop, waking every second.

### Safe Data Access
```c
pthread_mutex_lock(&g_metrics_mutex);
snapshot = g_metrics;            // copy the struct in one shot
pthread_mutex_unlock(&g_metrics_mutex);
// ... all printf calls use 'snapshot', not g_metrics
```
The mutex is released *before* any printing. This is the correct producer/consumer pattern — the collector is never blocked by slow terminal I/O.

### Screen Refresh
```c
printf("\033[H\033[J");
```
ANSI escape codes: move cursor to home (`\033[H`) then erase the screen (`\033[J`). Gives a clean dashboard effect without needing the `ncurses` library.

---

## Phase 4 — `main.c` & `Makefile` · Entry Point & Build

### `main.c`
- `PTHREAD_MUTEX_INITIALIZER` — statically initializes the mutex at compile time; no `pthread_mutex_init()` call needed.
- Creates both threads with `pthread_create`, then blocks on `pthread_join` — keeping the process alive until killed with Ctrl+C.

### `Makefile`
```makefile
$(TARGET): $(SRCS) monitor.h
    $(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lpthread
```
`monitor.h` is listed as a dependency — if the header changes, `make` knows to recompile everything. `-lpthread` links the POSIX thread library.

---

## Possible Improvements (Impressive but Not Overengineering)

These are real, interview-worthy upgrades — each one is a focused, self-contained addition.

---

### 1. Add a Log File (1–2 hours)
Write each metric snapshot to a `.csv` file alongside the live display. Trivial to add in `collector.c`:
```c
fprintf(logfile, "%ld,%.1f,%ld,%.1f,%d\n",
        time(NULL), cpu, mem, temp, sensor);
```
**Why it impresses:** Shows awareness of observability and data persistence — a real requirement in any IoT deployment.

---

### 2. Add a Configurable Sample Rate via CLI (30 min)
Instead of hardcoded `sleep(1)`, pass the interval as `argv[1]`:
```c
int interval = argc > 1 ? atoi(argv[1]) : 1;
```
**Why it impresses:** Shows you understand that parameters shouldn't be magic numbers — a basic but important embedded systems principle.

---

### 3. Add an Alert Threshold (1–2 hours)
Print a warning line if CPU exceeds a threshold:
```c
if (snapshot.cpu_usage > 80.0f)
    printf("  !! WARNING: High CPU usage detected !!\n");
```
**Why it impresses:** Turns a passive monitor into a basic alerting system — exactly what a real IoT monitor does.

---

### 4. Read Multiple Thermal Zones (1 hour)
Instead of only `thermal_zone0`, loop through `thermal_zone0` to `thermal_zone4` and report the max:
```c
for (int i = 0; i < 5; i++) {
    snprintf(path, sizeof(path),
             "/sys/class/thermal/thermal_zone%d/temp", i);
    // open and read...
}
```
**Why it impresses:** Shows knowledge of the Linux sysfs tree and that real hardware has multiple sensors.

---

### 5. Add a Graceful Shutdown with `SIGINT` (1 hour)
Instead of relying on Ctrl+C to kill the process hard, catch the signal and set a flag:
```c
volatile sig_atomic_t g_running = 1;

void handle_sigint(int sig) { g_running = 0; }

// In thread loops: while (g_running) { ... }
```
**Why it impresses:** Demonstrates understanding of signal handling and clean resource teardown — critical in embedded and production systems.

---

## Resume Mapping

| Resume Line | Where in Code |
|---|---|
| Lightweight device monitoring tool in C | `main.c`, `Makefile` — no external libs, pure C99 |
| Collected metrics using `/proc` and `/sys` | `collector.c` — `read_cpu_usage()`, `read_free_memory_mb()`, `read_temperature()` |
| Multithreading for periodic collection & display | `pthread_create` in `main.c`, both thread functions |
| Modular architecture | 4 separate files, each with a single responsibility |
| Simulated IoT sensor data | `rand()` in `collector.c` |
