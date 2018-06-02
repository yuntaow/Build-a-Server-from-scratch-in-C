CC=gcc
all: 
	$(CC) -pthread -o server server.c

clean:
	rm server