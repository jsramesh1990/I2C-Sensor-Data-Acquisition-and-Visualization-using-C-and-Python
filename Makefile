CC=gcc
CFLAGS=-Wall -O2 -pthread
LIBS=-lsqlite3 -lm

all: backend

backend: backend.c
	$(CC) $(CFLAGS) backend.c -o backend $(LIBS)

clean:
	rm -f backend *.o
	rm -f /tmp/sensor_backend.sock sensor_data.db

