CC = gcc
CFLAGS = -Wall -Wextra -g -O0

main: main.c
	$(CC) $(CFLAGS) -o main main.c

clean:
	rm -f main actual.txt

test: main
	gdb -batch -nx -x test.gdb ./main 2>&1 | grep -E '^(\$$[0-9]+ = |bucket )' > actual.txt
	diff expected.txt actual.txt

.PHONY: clean test