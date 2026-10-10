CC = gcc
CFLAGS = -Wall -Wextra -g -O0

main: main.c
	$(CC) $(CFLAGS) -o main main.c

clean:
	rm -f main

.PHONY: clean