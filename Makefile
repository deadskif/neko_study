CC = gcc
CFLAGS = -W -Wall -O2
.PHONY: all test
all: kotya
kotya: Makefile main.c
	$(CC) $(CFLAGS) main.c kotya_server.c -o $@
test:
	echo -en "GET / HTTP/1.1\r\n\r\n" | nc -t 127.0.0.1 8080
	echo -en "GET /im.jpg HTTP/1.1\r\n\r\n" | nc -t 127.0.0.1 8080

