#pragma once

void *_allocate_memory(size_t bytes){
    void *ptr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr; 
}

void _deallocate_memory(void *addr, size_t len){
    munmap(addr, len);
}