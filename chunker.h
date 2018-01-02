#pragma once

#include "./common.h"

enum allocator_type {BIG_BLOCK};

typedef struct{
    // void *start; start is exacly where is this struct
    size_t num_pages;
    enum allocator_type allocator_type;
    struct chunk_desc *next;
} chunk_desc;


size_t page_size;


chunk_desc* first_chunk = NULL;

void register_chunk(chunk_desc *new_ch){
    new_ch->next = first_chunk;
    first_chunk->next = new_ch;
}



void some_asserts(){
    assert(sizeof(chunk_desc) % sizeof(void *) == 0);
}

__attribute__((constructor)) void chunker_init(){
    page_size = sysconf(_SC_PAGESIZE);
    some_asserts();
}