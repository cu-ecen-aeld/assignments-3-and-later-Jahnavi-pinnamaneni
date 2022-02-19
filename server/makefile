CC = gcc
all: aesdsocket
.DEFAULT_GOAL := aesdsocket

aesdsocket: aesdsocket.c 
	$(CROSS_COMPILE)$(CC) aesdsocket.c -o aesdsocket

clean:
	rm -f *.o aesdsocket