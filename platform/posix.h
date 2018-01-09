#pragma once

#include <sys/mman.h>


void *allocate_memory(size_t bytes){
    // todo I assume this is zero-filled
    // "on Linux, the mapping will be created at a nearby page boundary"
    assert(bytes % getpagesize() == 0);
    void *ptr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != (void *)-1);
    return ptr; 
}

void deallocate_memory(void *addr, size_t len){
    assert(!munmap(addr, len));
}