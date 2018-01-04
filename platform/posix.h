#pragma once

#include <sys/mman.h>


void *allocate_memory(size_t bytes){
    // todo I assume this is zero-filled
    void *ptr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr; 
}

void deallocate_memory(void *addr, size_t len){
    assert(!munmap(addr, len));
}