#include "./common.h"
#include "./memory.h"
#include "./allocator/big_block.h"
#include "./allocator/bitmap.h"
#include "./allocator/ff.h"


mutex_t mutex;


allocator *choose_allocator_by_size(size_t size){
    if(size <= 128){
        return &bitmap_allocator;
    }

    if(size <=  3000){
        return &ff_allocator;
    }

    return &big_block_allocator;
}



void *my_alloc(size_t size, size_t align){
    { // ensure sizeof(void *) alignment
        size = size + sizeof(void *) - (size % sizeof(void *)); 
    }

    void *ptr = choose_allocator_by_size(size)->alloc(size, align); // choose allocator apropriate to size of block
    return ptr;
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

    chunk $ free(old_data_ptr);

    return new_ptr;
}


__attribute__((constructor)) void memory_init(){
    mutex_init(&mutex);
}



void my_free(void *ptr){
    if(ptr == NULL){
        return;
    }

    mutex_lock(&mutex);

    chunk_header *chunk = chunk_find_by_data_ptr(ptr);
    chunk $ free(ptr);

    mutex_unlock(&mutex);

    assert(chunk != NULL);
}


void *my_realloc(void *ptr, size_t size){
    // "If ptr is NULL, then the call is equivalent to malloc(size), for all values of size"
    if(ptr == NULL){
        void *ret = my_malloc(size);
        return ret;
    }

    // "if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr)"
    if(size == 0 && ptr != NULL){
        my_free(ptr);
    }

    assert(ptr != NULL);

    mutex_lock(&mutex);
    chunk_header *chunk = chunk_find_by_data_ptr(ptr);
    

    if(my_try_resize(chunk, ptr, size)){
        mutex_unlock(&mutex);
        return ptr; // resized!
    }

    size_t old_size = chunk $ data_size(ptr);
    if(old_size != 0 && size > old_size){
        void *ret = my_move_to_bigger_block(ptr, chunk, old_size, size);
        mutex_unlock(&mutex);
        return ret;
    }

    mutex_unlock(&mutex);
    return NULL;
}



void *my_malloc(size_t size){
    mutex_lock(&mutex);
    void *ret = my_alloc(size, sizeof(void *));
    mutex_unlock(&mutex);
    return ret;
}

void *my_calloc(size_t count, size_t size){
    size *= count;

    mutex_lock(&mutex);

    void *mem = my_alloc(size, sizeof(void *));

    mutex_unlock(&mutex);

    if(mem != NULL){
        memset(mem, 0, size);
    }
    return mem;
}

int my_posix_memalign(void **memptr, size_t alignment, size_t size){
    mutex_lock(&mutex);
    *memptr = my_alloc(size, alignment);
    mutex_unlock(&mutex);
    return 0;
}

void my_mdump(){
    mutex_lock(&mutex);
    printf("\n\n----| DUMPING ALL ALLOCATOR DATA |----\n");
    chunk_mdump();
    printf("----|        DONE DUMPING        |----\n\n\n\n\n\n\n\n\n\n");
    mutex_unlock(&mutex);
}