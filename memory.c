// #define NDEBUG


#include "./common.h"
#include "./memory.h"
#include "./allocator/big_block.h"
#include "./allocator/bitmap.h"
#include "./allocator/ff.h"

#define $ ->_->


allocator *choose_allocator_by_size(size_t size){
    // if(size <= 512){
    //     return &bitmap_allocator;
    // printf("bm\n");
    // }

    if(size <=  3000){
    // printf("ff\n");
        return &ff_allocator;
    }

    // printf("bb\n");
    return &big_block_allocator;
}



void *my_alloc(size_t size, size_t align){
    { // ensure sizeof(void *) alignment
        assert(align % sizeof(void *) == 0); 
        size = size + sizeof(void *) - (size % sizeof(void *)); 
    }

    void *ptr = choose_allocator_by_size(size)->alloc(size, align); // choose allocator apropriate to size of block
    return ptr;
}


extern void my_free(void *ptr){
    assert(ptr != NULL);
    chunk_header *chunk = chunk_find_by_data_ptr(ptr);
    assert(chunk != NULL);
    chunk $ free(ptr);
}

// resize tries to resize block, if moving is needed, then returns false
bool my_try_resize(chunk_header *chunk, void *ptr, size_t size){
    assert(chunk != NULL);
    return chunk $ try_resize(ptr, size);
}

void *my_move_to_bigger_block(void *old_data_ptr, chunk_header *chunk, size_t old_size, size_t size){
    void *new_ptr = my_alloc(size, sizeof(void *));
    if(new_ptr){
        assert(old_size < size);
        memcpy(new_ptr, old_data_ptr, old_size);
    }

    my_free(old_data_ptr);

    return new_ptr;
}

extern void *my_realloc(void *ptr, size_t size){
    // "If ptr is NULL, then the call is equivalent to malloc(size), for all values of size"
    if(ptr == NULL){
        return my_malloc(size);
    }

    // "if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr)"
    if(size == 0 && ptr != NULL){
        my_free(ptr);
    }

    assert(ptr != NULL);

    chunk_header *chunk = chunk_find_by_data_ptr(ptr);

    if(my_try_resize(chunk, ptr, size)){
        return ptr; // resized!
    }

    size_t old_size = chunk $ data_size(ptr);

    if(old_size != 0 && size > old_size){
        return my_move_to_bigger_block(ptr, chunk, old_size, size);
    }

    return NULL;
}



extern void *my_malloc(size_t size){
    return my_alloc(size, sizeof(void *));
}

extern void *my_calloc(size_t count, size_t size){
    size *= count;
    void *mem = my_alloc(size, sizeof(void *));
    if(mem != NULL){
        memset(mem, 0, size);
    }
    return mem;
}

extern int my_posix_memalign(void **memptr, size_t alignment, size_t size){
    *memptr = my_alloc(size, alignment);
    return 0;
}