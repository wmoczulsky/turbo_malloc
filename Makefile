
test: tests/functional.c
	gcc -std=gnu11 -g -Wall -Wextra tests/functional.c -o 'test' -lpthread  -lrt 
