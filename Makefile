CC = gcc
CFLAGS = -W -Wall -O2
kotya: Makefile main.c
	$(CC) $(CFLAGS) main.c -o $@
