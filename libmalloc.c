#include <unistd.h>
#include "memory.h"

void free(void *ptr){
    my_free(ptr);
}

void *realloc(void *ptr, size_t size){
    return my_realloc(ptr, size);
}

void *malloc(size_t size){
    return my_malloc(size);
}

void *calloc(size_t count, size_t size){
    return my_calloc(count, size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size){
    return my_posix_memalign(memptr, alignment, size);
}
