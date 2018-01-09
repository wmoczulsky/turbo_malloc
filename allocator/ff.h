#pragma once

#include "../common.h"
#include "../chunk.h"
#include "./abstract.h"

#define let size_t

allocator ff_allocator;

struct free_block_data {
    struct ff_block *prev_free;
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
    // each ff_region has exacly size of 8 pages - sizeof(ff_region)

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

void ff_set_block_size(ff_block *block, uint16_t size){
    block->size_and_free = (block->size_and_free & 1) | (size << 1);
}

void ff_block_set_is_free(ff_block *block, bool value){
    block->size_and_free = (block->size_and_free & 0xFFFE) + value;
}

ff_region *ff_new_region(){
    ff_region *new_region = allocate_chunk(8 * getpagesize() - sizeof(chunk_header), &ff_allocator);

    new_region->next = ff_first_region;
    ff_first_region = new_region;

    INIT_CANARY(new_region, ff_region);

    // init block inside:
    ff_block *block = (void *)new_region + sizeof(ff_region);
    new_region->first_free = block;
    ff_block_set_is_free(block, true);
    INIT_CANARY(block, ff_block);
    ff_set_block_size(block, 8 * getpagesize() - sizeof(chunk_header) - sizeof(ff_region));

    return new_region;
}

size_t ff_calc_shift(void *ptr, size_t align){
    size_t excess = (size_t)ptr % align;
    return (align - excess) % align;
}

bool ff_block_is_spacious_enough(ff_block *block, size_t size, size_t align){
    assert(ff_is_block_free(block));

    let space = ff_get_block_size(block) - ((size_t)block->data - (size_t)block);

    void *start = block->data;

    let shift = ff_calc_shift(start, align);

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
        if(ff_is_block_free(block) && ff_block_is_spacious_enough(block, size, align)){
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
            CHECK_CANARY(block, ff_block);
            return block;
        }
        region = region->next;
    }

    region = ff_new_region();
    ff_block *block = ff_find_free_block_in_region(region, size, align);
    return block;
}

void ff_split_free_block(ff_block *block, size_t size){
    //TODO
}

void ff_mark_as_used(ff_block *block, size_t size){
    CHECK_CANARY(block, ff_block);

    if(ff_get_block_size(block) - size > sizeof(ff_block)){
        // this is to split block
        ff_split_free_block(block, size);
    }

    assert(ff_get_block_size(block) >= size);
    assert(ff_is_block_free(block));

    ff_set_block_size(block, size);
    ff_block_set_is_free(block, false);

    ff_block *prev_free = block->free_block_data.prev_free;
    ff_block *next_free = block->free_block_data.next_free;

    if(prev_free != NULL)
        prev_free->free_block_data.next_free = next_free;

    if(next_free != NULL)
    next_free->free_block_data.prev_free = prev_free;
}


void *ff_alloc(size_t size, size_t align){
    if(size < sizeof(((ff_block *)0)->free_block_data)){
        size = sizeof(((ff_block *)0)->free_block_data); // this makes it possible to mark this block as free in future
    }

    ff_block * block = ff_find_free_block_to_alloc(size, align);

    if(block == NULL){
        return NULL;
    }

    let shift = ff_calc_shift(block->data, align);

    ff_mark_as_used(block, size + shift);

    return (void *)block + shift;
}

void ff_free(void *ptr){

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

    