#pragma once
#include "../common.h"
#include "../chunk.h"



typedef struct {
    void *(*alloc)(size_t size, size_t align);
    void (*free)(void *ptr);
    bool (*try_resize)(void *ptr, size_t new_size);
    size_t (*data_size)(chunk_header *ptr);

} allocator;