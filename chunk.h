#pragma once

#include "./common.h"


typedef struct chunk_desc{
    // this struct is always at the beginning of chunk
    size_t num_pages;
    enum allocator_type allocator_type;
    struct chunk_desc *next;

} chunk_desc;


size_t page_size;


chunk_desc* first_chunk = NULL;



void register_chunk(chunk_desc *new_ch){
    new_ch->next = first_chunk;
    first_chunk = new_ch;
}
void unregister_chunk(chunk_desc *which){
    if(first_chunk == which){
        first_chunk = first_chunk->next;
        return;
    }

    chunk_desc *i = first_chunk;
    assert(i != NULL); // out of the list! 
    while(i->next != which){
        i = i->next;
        assert(i->next != NULL); // out of the list! 
    }

    i->next = i->next->next;
}



chunk_desc *chunk_find_by_data_ptr(void *data_ptr){
    assert(first_chunk != NULL);
    chunk_desc *i = first_chunk;
    while(data_ptr < (void *)i || data_ptr > (void *)i + i->num_pages * page_size){
        i = i->next;
        assert(i != NULL);
    }
    return i;
}



void some_asserts(){
    assert(sizeof(chunk_desc) % sizeof(void *) == 0);
}

__attribute__((constructor)) void chunker_init(){
    page_size = sysconf(_SC_PAGESIZE);
    some_asserts();
}