#pragma once

#include "./common.h"
#include "./chunker.h"


void *big_block_alloc(size_t size, size_t align){
    // layout:
    // [chunk_desc][?align?][   d    a    t    a   ]

    size_t max_size = sizeof(chunk_desc) + align + size;
    size_t num_pages = (max_size + page_size - 1) / page_size;

    void *ptr = _allocate_memory(num_pages * page_size);

    if(ptr == NULL){
        return NULL;
    }

    void *end_of_chunk_desc = ptr + sizeof(chunk_desc); 

    void *data = end_of_chunk_desc + align - ((size_t)end_of_chunk_desc % align);

    chunk_desc* desc = ptr;
    desc->num_pages = num_pages;
    desc->allocator_type = BIG_BLOCK;
    desc->data_ptr = data;
    register_chunk(ptr);

    assert(data + size <= num_pages * page_size + ptr);
    assert((size_t)data % align == 0);

    return data;
}

void big_block_free(chunk_desc *chunk){
    assert(chunk->allocator_type == BIG_BLOCK);
    unregister_chunk(chunk);
    _deallocate_memory(chunk, chunk->num_pages * page_size);
}