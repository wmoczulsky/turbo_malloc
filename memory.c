#include "./common.h"
#include "memory.h"
#include "big_block.h"





void *my_alloc(size_t size, size_t align){

    { // ensure sizeof(void *) alignment
        assert(align % sizeof(void *) == 0); 
        size = size + sizeof(void *) - (size % sizeof(void *)); 
    }

    return big_block_alloc(size, align); // choose allocator apropriate to size of block
}


void my_free(void *ptr){
    chunk_desc *chunk = find_chunk_by_data_ptr(ptr);
    big_block_free(chunk);
}




extern void *my_malloc(size_t size){
    return my_alloc(size, sizeof(void *));
}

extern void *my_calloc(size_t count, size_t size){
    size *= count;
    void *mem = my_alloc(size, sizeof(void *));
    memset(mem, 0, size);
    return mem;
}

extern int my_posix_memalign(void **memptr, size_t alignment, size_t size){
    memptr = my_alloc(size, alignment);
    return 0;
}