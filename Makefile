CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = iot_monitor
SRCS    = main.c collector.c display.c

$(TARGET): $(SRCS) monitor.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lpthread

clean:
	rm -f $(TARGET)
