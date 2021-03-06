#pragma once

#include "./common.h"
#include "./allocator/abstract.h"

// Chunk layout:
// [chunk_header][....................................................................................]

typedef struct chunk_header {
    // this struct is always at the beginning of chunk
    CANARY_START; 

    size_t num_pages;
    struct chunk_header *next;
    allocator *_; // virtual method table

    CANARY_END; 
} chunk_header;


chunk_header* first_chunk = NULL;

void register_chunk(chunk_header *new_ch){
    new_ch->next = first_chunk;
    first_chunk = new_ch;
}

void *allocate_chunk(size_t min_len, allocator *VMD){ 
    // min_len means usable memory - except header


    size_t num_pages = (min_len + sizeof(chunk_header) + getpagesize() - 1) / getpagesize();

    chunk_header *chunk = allocate_memory(num_pages * getpagesize());
    INIT_CANARY(chunk, chunk_header);
    chunk->num_pages = num_pages;
    chunk->_ = VMD;

    register_chunk(chunk);

    assert((size_t)((void *)chunk + sizeof(chunk_header)) % sizeof(void *) == 0);

    return (void *)chunk + sizeof(chunk_header);
}


void unregister_chunk(chunk_header *which){
    CHECK_CANARY(which, chunk_header);
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
    CHECK_CANARY(which, chunk_header);

    unregister_chunk(which);
    deallocate_memory(which, which->num_pages * getpagesize());
}



chunk_header *chunk_find_by_data_ptr(void *data_ptr){
    assert(first_chunk != NULL);
    assert(data_ptr != NULL);
    chunk_header *i = first_chunk;
    while(data_ptr < (void *)i || data_ptr > (void *)i + i->num_pages * getpagesize()){
        i = i->next;
        assert(i != NULL);
    }

    CHECK_CANARY(i, chunk_header);
    return i;
}


void chunk_mdump(){
    chunk_header *i = first_chunk;
    while(i != NULL){
        printf("- Chunk with %zu pages\n", i->num_pages);

        i $ mdump((void *)i + sizeof(chunk_header));
        printf("\n");
        i = i->next;
    }
}


void some_asserts(){
    assert(sizeof(chunk_header) % sizeof(void *) == 0);
}


// todo macro for constructors
__attribute__((constructor)) void chunker_init(){
    some_asserts();
}