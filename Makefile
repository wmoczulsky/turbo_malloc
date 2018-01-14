CC = clang
CFLAGS = -std=gnu11 -Wall -Wextra -g

all: test 

# main: main.c memory.o
# 	$(CC) $(CFLAGS) main.c memory.o -o main

memory.o: memory.c memory.h chunk.h common.h platform/posix.h platform/windows.h allocator/abstract.h allocator/big_block.h allocator/bitmap.h allocator/ff.h
	$(CC) $(CFLAGS) -c memory.c -o memory.o 

clean:
	rm -f main *.o *~ *.so ./test



test: tests/functional.c memory.o
	$(CC) $(CFLAGS) tests/functional.c memory.o -o 'test' -lpthread  -lrt 
