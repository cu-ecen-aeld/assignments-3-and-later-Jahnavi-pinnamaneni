CC = gcc
all: aesdsocket
.DEFAULT_GOAL := aesdsocket

aesdsocket: aesdsocket.c 
	$(CROSS_COMPILE)$(CC) aesdsocket.c -o aesdsocket -lpthread -lrt

clean:
	rm -f *.o aesdsocket