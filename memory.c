#include "./common.h"
#include "./memory.h"
#include "./big_block.h"


allocator *get_allocator_by(void *ptr){
    chunk_desc *chunk = chunk_find_by_data_ptr(ptr);
    switch(chunk->allocator_type){
        case BIG_BLOCK:
        return big_block_allocator;
    default:
        assert(0);
        return NULL;
    }
}



// void *my_alloc(size_t size, size_t align){

//     { // ensure sizeof(void *) alignment
//         assert(align % sizeof(void *) == 0); 
//         size = size + sizeof(void *) - (size % sizeof(void *)); 
//     }

//     void *ptr = big_block_alloc(size, align); // choose allocator apropriate to size of block

//     return ptr;
// }


// void my_free(void *ptr){
//     assert(ptr != NULL);
//     chunk_desc *chunk = find_chunk_by_data_ptr(ptr);

//     switch(chunk->allocator_type){
//         case BIG_BLOCK:
//             big_block_free(chunk);
//             break;
//         default:
//             assert(0);
//     }
// }

// // resize tries to resize block, if moving is needed, then return false
// bool my_try_resize(chunk_desc *chunk, size_t size){
//     switch(chunk->allocator_type){
//         case BIG_BLOCK:
//             return big_block_try_to_resize(chunk, size);
//             break;
//         default:
//             assert(0);
//     }
// }

// void *my_try_move_to_bigger_block(chunk_desc *chunk, size_t size){
//     void *new_ptr = my_alloc(size, sizeof(void *));
//     if(new_ptr){
//         assert(chunk->num_pages * page_size < size);
//         memcpy(new_ptr, chunk->data_ptr, chunk->data_size);
//     }

//     return new_ptr;
// }

// void *my_realloc(void *ptr, size_t size){
//     // "If ptr is NULL, then the call is equivalent to malloc(size), for all values of size"
//     if(ptr == NULL){
//         return my_malloc(size);
//     }

//     // "if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr)"
//     if(size == 0 && ptr != NULL){
//         my_free(ptr);
//     }

//     assert(ptr != NULL);

//     chunk_desc *chunk = chunk_find_by_ptr(ptr);

//     if(my_try_resize(chunk, size)){
//         return ptr; // resized!
//     }

//     if(size > chunk->data_size){
//         return my_try_move_to_bigger_block(chunk, size);
//     }

//     return NULL;
// }



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
    *memptr = my_alloc(size, alignment);
    return 0;
}