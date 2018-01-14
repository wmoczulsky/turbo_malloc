#pragma once

#include <sys/mman.h>
#include <pthread.h>


typedef pthread_mutex_t mutex_t;


void mutex_init(mutex_t *mutex){
    pthread_mutex_init(mutex, NULL);
}

int mutex_lock(mutex_t *mutex){
    return pthread_mutex_lock(mutex);
}

int mutex_trylock(mutex_t *mutex){
    return pthread_mutex_trylock(mutex);
}

int mutex_unlock(pthread_mutex_t *mutex){
    return pthread_mutex_unlock(mutex);
}



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