#pragma once

#include "../common.h"
#include "../chunk.h"
#include "./abstract.h"

allocator bitmap_allocator;

/*
Keeping list of regions, if alloc is impossible in one of them, allocate new region.
First fit over the list of regions.
Every region is a tree with h = 2
every leaf's bit tells if 16-bytes long cell is empty
every root's vit tells if there is any empty cell under the corresponding leaf
every region has exacly 16 pages (~64 kb). The first page is partly used by metadata
*/

typedef struct bitmap_region {
    // uint64_t magic = 1431124; // TODO
    struct region *next;
    uint64_t root;
    uint64_t leafs[64];

    PUT_CANARY(bitmap_region);
} region;


typedef struct bitmap_entry {
    uint8_t cells; 
    // 1 cell is equal to 16 bytes
    // so max size is 255 * 16 = 4080 bytes
} bitmap_entry;


bitmap_region first_bitmap_region = NULL;

inline bitmap_entry *give_memory(bitmap_region* region, void *addr, size_t size){
    // 
}

// try to fit block with the beginning in given leaf
inline bitmap_entry *try_fit_in_leaf(size_t leaf_number, bitmap_region* region, size_t cells, size_t align){
    for(size_t i = 0; i < 64; i++){
        if(get_bit(i, region->leafs[leaf_number])){
            void *addr = (void *)region + sizeof(bitmap_region) + 16 * 64 * leaf_number + 16 * i;
            if((size_t)addr % align == 0){
                // check if there is enough space:
                for(size_t j = 0; j < size + sizeof(bitmap_entry); j++){
                    uint8_t byte = ((uint8_t *)region->leafs[leaf_number]) + (j / 8);
                    if((byte >> (7 - j % 8))&1 == 0){
                        return NULL;
                    }
                }

                return give_memory(region, addr, size);
            }
        }
    }
    return NULL;
}


inline bool get_bit(size_t n, uint64_t where){
    return (where >> (63 - n)) & 1;
}

_Static_assert(getbit(5, 32))


bitmap_entry *find_place_in_region(bitmap_region *region, size_t size, size_t align){
    CHECK_CANARY(region, bitmap_region);
    // I know, that it could be optimized using instructions like __builtin_ffs, but I prefer to keep it simple
    // search is done from the left
    for(size_t i = 0; i < 64; i++){
        if(get_bit(i, region->root)){
            bitmap_entry *e = try_fit_in_leaf(i, region);
            if(bitmap_entry != NULL){
                return bitmap_entry;
            }
        }
    }

    return NULL;
}


bitmap_entry *bitmap_find_place(size_t size, size_t align){
    bitmap_region *i = first_bitmap_region;
    while(i != NULL){
        CHECK_CANARY(i, bitmap_region);

        bitmap_entry *place = find_place_in_region(i, size, align);
        if(place != NULL){
            return place;
        }

        i = i->next;
    }

    // request new region
    bitmap_region *new_region = allocate_memory(16 * page_size);
    new_region->next = first_bitmap_region;
    first_bitmap_region = new_region;
    return find_place_in_region(new_region, size, align);
}


void *bitmap_alloc(size_t size, size_t align){
    assert(size <= 4080);

    bitmap_entry *place = bitmap_find_place(size, align);
}

void bitmap_free(void *ptr){

}

bool bitmap_try_resize(void *ptr, size_t new_size){
    return false;
}

size_t bitmap_data_size(chunk_header *ptr){
    return 0;
}

allocator bitmap_allocator = {
    .alloc = bitmap_alloc,
    .free = bitmap_free,
    .try_resize = bitmap_try_resize,
    .data_size = bitmap_data_size
};