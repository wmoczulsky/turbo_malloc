#pragma once

#include "../common.h"
#include "../chunk.h"
#include "./abstract.h"

allocator big_block_allocator;

typedef struct {
    size_t data_size;
    // void *data_ptr;
} big_block_header;


void *big_block_alloc(size_t size, size_t align){
    // layout inside chunk:
    // [big_block_header][?align?][     d       a       t       a     ]

    size_t sum_size = sizeof(big_block_header) + align + size;

    big_block_header *header = allocate_chunk(sum_size, &big_block_allocator);

    if(header == NULL){
        return NULL;
    }

    header->data_size = size;
    // header->data_ptr

    void *header_end = (void *)header + sizeof(big_block_header);

    void *data = header_end + align - ((size_t)header_end % align);

    assert((size_t)data % align == 0);

    return data;
}

void big_block_free(void *ptr){
    free_chunk(chunk_find_by_data_ptr(ptr));
}

bool big_block_try_resize(void *ptr, size_t new_size){
    return false; // not implemented for this kind of allocation
}

size_t data_size(chunk_header *ptr){
    assert(ptr != NULL);
    big_block_header *hdr = (void *)ptr + sizeof(chunk_header);
    return hdr->data_size;
}

allocator big_block_allocator = {
    .alloc = big_block_alloc,
    .free = big_block_free,
    .try_resize = big_block_try_resize,
    .data_size = data_size
};