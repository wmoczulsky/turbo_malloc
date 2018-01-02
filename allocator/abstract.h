#pragma once
#include "../common.h"
#include "../chunk.h"



typedef struct{
    void *(*alloc)(size_t size, size_t align);
    void (*free)(void *ptr);
    bool (*try_resize)(void *ptr, size_t new_size);

} allocator;