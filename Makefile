CC = clang
CFLAGS = -std=gnu11 -Wall -Wextra

all: test 

main: main.c memory.o
	$(CC) $(CFLAGS) -g main.c memory.o -o main

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -g  -c memory.c -o memory.o 

clean:
	rm -f main *.o *~ *.so ./test



test: tests/functional.c memory.o
	clang -std=gnu11 -g -Wall -Wextra tests/functional.c memory.o -o 'test' -lpthread  -lrt 
