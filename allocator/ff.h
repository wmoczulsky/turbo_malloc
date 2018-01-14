#pragma once

#include "../common.h"
#include "../chunk.h"
#include "./abstract.h"

#define let size_t

allocator ff_allocator;

// TODO boundary tag

struct free_block_data {
    struct ff_block *prev_free; // todo shorter numbers instead of ptrs
    struct ff_block *next_free;
};

typedef struct ff_block { 
    CANARY_START;
    
    // least significant bit tells if block is free
    // rest of bits (15-bit unsigned int) tells size including this header 
    uint16_t size_and_free; 

    CANARY_END; 

    union {
        uint8_t data[0]; // data ptr
        struct free_block_data free_block_data;
    };
} ff_block;


typedef struct ff_region {
    CANARY_START;

    struct ff_region *next;
    struct ff_block *first_free;
    // each ff_region has exacly size of 1 page - sizeof(chunk_header)

    // everybody knows where is first block...

    CANARY_END; 
} ff_region;


ff_region *ff_first_region = NULL;


size_t ff_get_block_size(ff_block *block){
    return block->size_and_free >> 1;
}

bool ff_is_block_free(ff_block *block){
    return block->size_and_free & 1;
}

void ff_set_block_size(ff_block *block, size_t size){
    assert(size >= sizeof(ff_block));
    block->size_and_free = (block->size_and_free & 1) | (size << 1);
}

void ff_block_set_is_free(ff_block *block, bool value){
    block->size_and_free = (block->size_and_free & 0xFFFE) + value;
}

ff_region *ff_get_region_by_ptr(void *ptr){
    ff_region *region = (void *)((size_t)ptr - ((size_t)ptr % getpagesize()) + sizeof(chunk_header));
    CHECK_CANARY(region, ff_region);
    return region;
}


void ____________________________ff_assert_free_list_is_ok(void *ptr){
    // slow, but sometimes necessary ;/
    ff_region *region = ff_get_region_by_ptr(ptr);
    ff_block *i = region->first_free;
    int a = 0;
    ff_block *j = NULL;
    while(i != NULL){
        j = i;
        // printf("iteration %d\n", a++);
        // printf("forward: %p\n", i);
        assert(i->free_block_data.prev_free < i);
        // printf("%p %p \n", i, i->free_block_data.prev_free );
        assert(i->free_block_data.prev_free == NULL || (size_t)i->free_block_data.prev_free/getpagesize()== (size_t)i/getpagesize());
        assert(ff_is_block_free(i));
        i = i->free_block_data.next_free;
    }


    // navigate left to check if links are ok
    i = j;
    while(i != NULL){
        // printf("backwards: %p\n", i);
        j = i;
        i = i->free_block_data.prev_free;
    }
    printf("-------> j %p ff %p\n", j, region->first_free);

    assert(j == NULL || j == region->first_free);
}


ff_region *ff_new_region(){
    ff_region *new_region = allocate_chunk(getpagesize() - sizeof(chunk_header), &ff_allocator);

    new_region->next = ff_first_region;
    ff_first_region = new_region;

    INIT_CANARY(new_region, ff_region);

    // init block inside:
    ff_block *block = (void *)new_region + sizeof(ff_region);
    new_region->first_free = block;
    ff_block_set_is_free(block, true);
    INIT_CANARY(block, ff_block);
    ff_set_block_size(block, getpagesize() - sizeof(chunk_header) - sizeof(ff_region));

    return new_region;
}

size_t ff_calc_shift(void *ptr, size_t align){
    size_t excess = (size_t)ptr % align;
    return (align - excess) % align;
}

bool ff_block_is_spacious_enough(ff_block *block, int size, size_t align){
    assert(ff_is_block_free(block));

    int space = (int)ff_get_block_size(block) - sizeof(ff_block) + sizeof(struct free_block_data);

    void *start = block->data;

    int shift = ff_calc_shift(start, align);

    if(space - shift >= size){
        return true;
    }else{
        return false;
    }
}

ff_block *ff_find_free_block_in_region(ff_region *region, size_t size, size_t align){
    CHECK_CANARY(region, ff_region);
    ff_block *block = region->first_free;

    while(block != NULL){
        assert(ff_is_block_free(block));

        if(ff_block_is_spacious_enough(block, size, align)){
            return block;
        }

        block = block->free_block_data.next_free;
    }

    return NULL;
}


ff_block *ff_find_free_block_to_alloc(size_t size, size_t align){
    ff_region *region = ff_first_region;
    while(region != NULL){
        ff_block *block = ff_find_free_block_in_region(region, size, align);
        if(block != NULL){
            printf("%p\n", block);
            CHECK_CANARY(block, ff_block);
            return block;
        }
        region = region->next;
    }

    region = ff_new_region();
    ff_block *block = ff_find_free_block_in_region(region, size, align);

    CHECK_CANARY(block, ff_block);
    return block;
}


void ff_set_block_as_free_and_update_list(ff_block *block, ff_block *prev_free, ff_block *next_free){
    assert(prev_free < 1llu<<48); // it is ptr, not random value
    assert(next_free < 1llu<<48); // it is ptr, not random value

    assert(prev_free < block || prev_free == NULL);
    assert(next_free > block || next_free == NULL);


    ff_block_set_is_free(block, true);

    block->free_block_data.prev_free = prev_free;
    block->free_block_data.next_free = next_free;

    if(prev_free != NULL) 
        prev_free->free_block_data.next_free = block;

    if(next_free != NULL)
        next_free->free_block_data.prev_free = block;

    if(prev_free == NULL){
        ff_get_region_by_ptr(block)->first_free = block;
    }
}


void ff_split_free_block_if_profitable(ff_block *block, size_t alloc_size){
    ____________________________ff_assert_free_list_is_ok(block);
    CHECK_CANARY(block, ff_block);
    assert(ff_is_block_free(block));


    let old_size = ff_get_block_size(block);
    let new_left_block_size = alloc_size + sizeof(ff_block) - sizeof(struct free_block_data);
    let new_right_block_size = old_size - new_left_block_size;

    // printf("aq %u %u %u\n", alloc_size, old_size, new_left_block_size);
// printf("%u %u %u %u %u\n", old_size, new_left_block_size, new_right_block_size, alloc_size, sizeof(ff_block));
    if(old_size < new_left_block_size || new_right_block_size < sizeof(ff_block)){
        // not profitable, maybe not even possible
        return;
    }

    ff_block *new_block = (void *)block + new_left_block_size;

    assert((size_t)block / getpagesize() == ((size_t)block + old_size - sizeof(ff_block)) / getpagesize());
    assert((size_t)new_block / getpagesize() == (size_t)block / getpagesize());

    INIT_CANARY(new_block, ff_block);

    ff_set_block_size(block, new_left_block_size);
    ff_set_block_size(new_block, new_right_block_size);


    ff_block_set_is_free(new_block, true);

    ff_block *next_free = block->free_block_data.next_free;

    ff_set_block_as_free_and_update_list(new_block, block, next_free);


    ____________________________ff_assert_free_list_is_ok(block);
}

void ff_mark_as_used(ff_block *block, size_t alloc_size){
    ____________________________ff_assert_free_list_is_ok(block);
    CHECK_CANARY(block, ff_block);

    assert(ff_get_block_size(block) >= alloc_size);
    assert(ff_get_block_size(block) >= alloc_size + sizeof(ff_block) - sizeof(struct free_block_data));
    ff_split_free_block_if_profitable(block, alloc_size);

    ____________________________ff_assert_free_list_is_ok(block);
    printf("fs %u %u\n", ff_get_block_size(block) , alloc_size);
    assert(ff_get_block_size(block) >= alloc_size);
    assert(ff_is_block_free(block));
    
    ff_block_set_is_free(block, false);

    ff_block *prev_free = block->free_block_data.prev_free;
    ff_block *next_free = block->free_block_data.next_free;

    printf("b: %p pr: %p nx: %p ff:%p ffp:%p ff:n%p \n", block, prev_free, next_free, 
        ff_get_region_by_ptr(block)->first_free, 
        ff_get_region_by_ptr(block)->first_free->free_block_data.prev_free,
        ff_get_region_by_ptr(block)->first_free->free_block_data.next_free
        );
    assert(prev_free < block || prev_free == NULL);
    assert(next_free > block || next_free == NULL);

    if(prev_free != NULL){
        prev_free->free_block_data.next_free = next_free;
    }else{
        ff_region *region = ff_get_region_by_ptr(block);
        region->first_free = next_free;
    }

    if(next_free != NULL)
        next_free->free_block_data.prev_free = prev_free;

    ____________________________ff_assert_free_list_is_ok(block);


}


void *ff_alloc(size_t size, size_t align){
    assert(size <= getpagesize() - sizeof(chunk_header) - sizeof(ff_region));

    if(size < sizeof(((ff_block *)0)->free_block_data)){
        size = sizeof(((ff_block *)0)->free_block_data); // this makes it possible to mark this block as free in future
    }

    ff_block * block = ff_find_free_block_to_alloc(size, align);

    if(block == NULL){
        return NULL;
    }

    let shift = ff_calc_shift(block->data, align);
    // printf("\n");
    printf("a %u %d\n",ff_get_block_size(block),  size + shift);
        ____________________________ff_assert_free_list_is_ok(block);

    ff_mark_as_used(block, size + shift);
    ____________________________ff_assert_free_list_is_ok(block);

    for(size_t i = 0; i < shift; i++){
        // please read long comment several lines below
        *((uint8_t *)block->data + i) = 0xFF;
    }
    *((uint8_t *)block->data - 1) &= 0xFE; 
    #ifdef NDEBUG
       for(uint8_t *i = &block->size_and_free + sizeof(block->size_and_free); i < (uint8_t *)block->data; i++){
            *i = 0;
        }
    #else
        for(uint8_t *i = &block->canary_end + sizeof(block->canary_end); i < (uint8_t *)block->data; i++){
            *i = 0;
        }
    #endif




    ____________________________ff_assert_free_list_is_ok(block);
    return (void *)block->data + shift;
}

ff_block *ff_get_block_by_alloc_ptr(void *ptr){
    // Unfortunately pointer given to user may be shifted.
    // How to reclaim block address in an realiable way?
    // ff_block header is always ended by 0 bit. 
    // Either because of size_and_free set to 0 - not free
    // or because of canary pointer which is aligned, so last several bits are set to 0
    // every bit of shift is set to 1
    // so in order to reclaim block pointer, just move left up to first 0 bit
    // this sould not be slow, because align is usually really small

    // It probably could be optimized even more, because align is equal to power of 2, 
    // but knowing, that most of aligns are really small, it may be not better

    uint8_t *shift_pointer = ptr;
    while(*(shift_pointer - 1) == 0xFF){
        shift_pointer -= 1;
    }

    ff_block *block = (void *)shift_pointer - sizeof(ff_block) + sizeof(struct free_block_data);
    CHECK_CANARY(block, ff_block);

    return block;
}

ff_block *ff_get_previous_free(ff_block *block){
    ff_block *i = ff_get_region_by_ptr(block)->first_free;

    while(i != NULL && i < block){
        if(i->free_block_data.next_free == NULL || i->free_block_data.next_free > block){
            return i;
        }
        i = i->free_block_data.next_free;
    }

    return NULL;
}

ff_block *ff_get_next_free(ff_block *block){
    ff_block *i = ff_get_region_by_ptr(block)->first_free;

    while(i != NULL){
        if(i > block){
            return i;
        }
        i = i->free_block_data.next_free;
    }

    return NULL;
}

void ff_free(void *ptr){
    ____________________________ff_assert_free_list_is_ok(ptr);
    // ff_region *region = ff_get_region_by_ptr(ptr);
    printf("FREE %p\n", ptr);
    ff_block *block = ff_get_block_by_alloc_ptr(ptr);

    ff_block *prev_free = ff_get_previous_free(block);
    ff_block *next_free = prev_free == NULL ? ff_get_next_free(block) : prev_free->free_block_data.next_free;

    ff_set_block_as_free_and_update_list(block, prev_free, next_free);
    
    ____________________________ff_assert_free_list_is_ok(block);

    // ff_try_merge_with_siblings(block);   
}

bool ff_try_resize(void *ptr, size_t new_size){
    return false;
}

size_t ff_data_size(chunk_header *ptr){
    return 0;
}

allocator ff_allocator = {
    .alloc = ff_alloc,
    .free = ff_free,
    .try_resize = ff_try_resize,
    .data_size = ff_data_size
};

    