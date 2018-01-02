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

    chunk_desc* desc = ptr;
    desc->num_pages = num_pages;
    desc->allocator_type = BIG_BLOCK;

    void *end_of_chunk_desc = ptr + sizeof(chunk_desc); 

    void *data = end_of_chunk_desc + align - ((size_t)end_of_chunk_desc % align);

    assert(data + size <= num_pages * page_size + ptr);
    assert((size_t)data % align == 0);

    return data;
}

void big_block_free(void *ptr){
    chunk_desc *dsc_ptr = ptr;
    assert(dsc_ptr->allocator_type == BIG_BLOCK);
    
}