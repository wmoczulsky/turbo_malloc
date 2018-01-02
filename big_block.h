#pragma once

#include "./common.h"
#include "./chunk.h"


typedef struct {
    size_t data_size;
    void *data_ptr;
} big_block_header;


void *big_block_alloc(size_t size, size_t align){
    // layout inside chunk:
    // [big_block_header][?align?][   d    a    t    a   ]

    size_t max_size = sizeof(chunk_desc) + align + size;
    size_t num_pages = (max_size + page_size - 1) / page_size + 10;

    void *ptr = allocate_memory(num_pages * page_size);

    if(ptr == NULL){
        return NULL;
    }

    void *end_of_chunk_desc = ptr + sizeof(chunk_desc); 

    void *data = end_of_chunk_desc + align - ((size_t)end_of_chunk_desc % align);

    chunk_desc* desc = ptr;
    desc->num_pages = num_pages;
    desc->allocator_type = BIG_BLOCK;
    desc->data_ptr = data;
    desc->data_size = size;
    register_chunk(ptr);

    assert(data + size <= num_pages * page_size + ptr);
    assert((size_t)data % align == 0);

    return data;
}

void big_block_free(void *ptr){
    chunk_data *chunk = chunk_find_by_data_ptr(ptr);
    assert(chunk->allocator_type == BIG_BLOCK);
    unregister_chunk(chunk);
    deallocate_memory(chunk, chunk->num_pages * page_size);
}

bool big_block_try_resize(void *ptr, size_t new_size){
    return false; // not implemented for this kind of allocation
}



allocator big_block_allocator = {
    .alloc = big_block_alloc,
    .free = big_block_free,
    .try_resize = big_block_try_resize
}