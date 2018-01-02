#pragma once

#include "./common.h"

// Chunk example:
// [chunk_header][....................................................................................]


typedef struct chunk_header{
    // this struct is always at the beginning of chunk
    size_t num_pages;
    enum allocator_type allocator_type;
    struct chunk_header *next;
} chunk_header;


size_t page_size;


chunk_header* first_chunk = NULL;

void register_chunk(chunk_header *new_ch){
    new_ch->next = first_chunk;
    first_chunk = new_ch;
}

void *allocate_chunk(size_t min_len, enum allocator_type allocator_type){ 
    // min_len means usable memory - except header

    size_t num_pages = (min_len + sizeof(chunk_header) + page_size - 1) / page_size;

    chunk_header *chunk = allocate_memory(num_pages * page_size);

    chunk->num_pages = num_pages;
    chunk->allocator_type = allocator_type;

    register_chunk(chunk);

    assert((size_t)((void *)chunk + sizeof(chunk_header)) % sizeof(void *) == 0);

    return (void *)chunk + sizeof(chunk_header);
}


void unregister_chunk(chunk_header *which){
    if(first_chunk == which){
        first_chunk = first_chunk->next;
        return;
    }

    chunk_header *i = first_chunk;
    assert(i != NULL); // out of the list! 
    while(i->next != which){
        i = i->next;
        assert(i->next != NULL); // out of the list! 
    }

    i->next = i->next->next;
}


void free_chunk(chunk_header *which){

    unregister_chunk(which);
    deallocate_memory(which, which->num_pages * page_size);
}



chunk_header *chunk_find_by_data_ptr(void *data_ptr){
    assert(first_chunk != NULL);
    chunk_header *i = first_chunk;
    while(data_ptr < (void *)i || data_ptr > (void *)i + i->num_pages * page_size){
        i = i->next;
        assert(i != NULL);
    }
    return i;
}



void some_asserts(){
    assert(sizeof(chunk_header) % sizeof(void *) == 0);
}

__attribute__((constructor)) void chunker_init(){
    page_size = sysconf(_SC_PAGESIZE);
    some_asserts();
}