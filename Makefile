CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lpthread

TARGET  = sleepingStylistMon

all: $(TARGET)

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c

$(TARGET): sleepingStylistMon.c monitor.o monitor.h
	$(CC) $(CFLAGS) -o $(TARGET) sleepingStylistMon.c monitor.o $(LDFLAGS)

clean:
	rm -f $(TARGET) monitor.o
