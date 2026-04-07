CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lpthread

TARGET  = sleepingStylistSem

all: $(TARGET)

$(TARGET): sleepingStylistSem.c
	$(CC) $(CFLAGS) -o $(TARGET) sleepingStylistSem.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
