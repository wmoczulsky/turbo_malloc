#pragma once

#include "../common.h"
#include "../chunk.h"
#include "./abstract.h"

#define let size_t

allocator bitmap_allocator;

/*
Keeping list of regions, if alloc is impossible in one of them, allocate new region.
First fit over the list of regions.
Every region is a tree with h = 2
every leaf's bit tells if 16-bytes long cell is empty
every root's bit tells if there is any empty cell under the corresponding leaf
every region has exacly 16 pages (~64 kb). The first page is partly used by metadata

*/

typedef struct bitmap_region {
    CANARY_START;

    struct bitmap_region *next;
    uint64_t root;
    uint64_t leafs[64];
    size_t allocs_count;

    CANARY_END; 
} bitmap_region;


typedef struct bitmap_entry {
    uint8_t cells;  // including this header
    // 1 cell is equal to 16 bytes
    // so max size is probably (255 - 1) * 16 = 4064 bytes
    // one cell probably is used as bitmap_entry header

} bitmap_entry;


bitmap_region *bitmap_first_region = NULL;




bool bitmap_get_bit(size_t n, uint64_t where){
    return (where >> (63llu - n)) & 1;
}



void *bitmap_give_memory(bitmap_region* region, void *addr, size_t cells){
    // mark as used
    let cell_i = (addr - (void *)region) / 16;
    for(let cell = cell_i; cell < cell_i + cells; cell++){
        let leaf_nr = cell / 64;
        assert(bitmap_get_bit(leaf_nr, region->root) == 1);

        let cell_in_leaf = cell % 64;
        assert(bitmap_get_bit(cell_in_leaf, region->leafs[leaf_nr]) == 1);

        // mark leaf cells as used
        region->leafs[leaf_nr] &= ~(1llu << (63 - cell_in_leaf));
        assert(bitmap_get_bit(cell_in_leaf, region->leafs[leaf_nr]) == 0);

        
        if(region->leafs[leaf_nr] == 0){
            region->root &= ~(1llu << (63 - leaf_nr));
        }
    }

    region->allocs_count += 1;
    return addr;
}

// try to fit block with the beginning in given leaf
void *bitmap_try_fit_in_leaf(size_t leaf_number, bitmap_region* region, size_t cells, size_t align){
    for(size_t i = 0; i < 64; i++){ // for every starting position in leaf
            void *addr = (void *)region + 16 * 64 * leaf_number + 16 * i;
            if(((size_t)addr + sizeof(bitmap_entry) ) % align == 0){  // if good align
                bool good = true;
                // check if there is enough space:
                for(size_t cell = 0; cell < cells; cell++){
                    // printf("%d\n", cell);
                    if(leaf_number + cell / 64 > 63 // out of boundary
                        || bitmap_get_bit((i + cell) % 64, region->leafs[leaf_number + (i + cell) / 64]) == 0){ // not empty cell
                        good = false;
                        break;
                    }
                }

                if(good){
                    // printf("giving memory %u %p %u %u\n", leaf_number, region, cells, align);
                    return bitmap_give_memory(region, addr, cells);
                }
            }
    }
    return NULL;
}



void *bitmap_find_place_in_region(bitmap_region *region, size_t cells, size_t align){
    CHECK_CANARY(region, bitmap_region);
    // I know, that it could be optimized using instructions like __builtin_ffs, but I prefer to keep it simple
    // search is performed from the left
    for(size_t i = 0; i < 64; i++){
        if(bitmap_get_bit(i, region->root)){
            bitmap_entry *e = bitmap_try_fit_in_leaf(i, region, cells, align);
            if(e != NULL){
                return e;
            }
        }
    }

    return NULL;
}

size_t get_chunk_header_shift(){
    // ensure our pointer will be 16 - bytes aligned
    // so ptr + sizeof(bitmap_region) + n * (16 * x + sizeof(bitmap_entry)) == 16
    // ptr + sizeof(bitmap_region) + n * (sizeof(bitmap_entry)) == 16 
    return 16 - (sizeof(chunk_header) + sizeof(bitmap_entry)) % 16;
}

bitmap_region *bitmap_make_new_region(){
 
    let shift = get_chunk_header_shift();
    let chunk_header_size = sizeof(chunk_header) + shift;


    // request new region, adding it to the list 
    bitmap_region *new_region = allocate_chunk(16 * page_size - chunk_header_size, &bitmap_allocator);
    if(new_region == NULL){
        return NULL;
    }

    new_region = (void *)new_region + shift; // apply shift fixing align

    assert(((size_t)new_region + sizeof(bitmap_entry)) % 16 == 0);

    // mark as all empty; it would be easier to use 0 as empty indicator, but nvm
    new_region->root = ~0ull;

    for(let i = 0; i < 64; i++){
        new_region->leafs[i] = ~0ull;
    }

    // because of chunk_header_size, we didn't get whole 16 pages. 
    // received pointer points to something after the beginning of the first page
    // we must mark proper number of cells at the end as unusable
    bitmap_give_memory(new_region, (void *)new_region + 16 * getpagesize() - chunk_header_size, (chunk_header_size + 16 - 1) / 16);


    bitmap_give_memory(new_region, (void *)new_region, (sizeof(bitmap_region) + 16 - 1) / 16); // mark bitmap_header as unusable


    INIT_CANARY(new_region, bitmap_region);
    new_region->next = bitmap_first_region;
    bitmap_first_region = new_region;
    // printf("new region: %p\n", new_region);
    return new_region;
}


bitmap_entry *bitmap_find_place(size_t size, size_t align){
    bitmap_region *i = bitmap_first_region;

    size_t cells = (size + sizeof(bitmap_entry) + 16 - 1) / 16;

    while(i != NULL){
        CHECK_CANARY(i, bitmap_region);

        bitmap_entry *place = bitmap_find_place_in_region(i, cells, align);
        if(place != NULL){
            place->cells = cells;
            return place;
        }

        i = i->next;
    }

    bitmap_region *new_region = bitmap_make_new_region();
    if(new_region == NULL){
        return NULL;
    }

    bitmap_entry *place = bitmap_find_place_in_region(new_region, cells, align);
    if(place != NULL){
        place->cells = cells;
    }

    return place;
}


void *bitmap_alloc(size_t size, size_t align){
    assert(size <= 4064);
    if(align < 16){
        align = 16; // align is power of 2
    }
    bitmap_entry *place = bitmap_find_place(size, align);
    return (void *)place + sizeof(bitmap_entry);
}

void bitmap_free(void *ptr){
    chunk_header *chunk = chunk_find_by_data_ptr(ptr);
    bitmap_region *region = (void *)chunk + sizeof(chunk_header) + get_chunk_header_shift();
    CHECK_CANARY(region, bitmap_region);

    // mark as unused 
    bitmap_entry *entry = ptr - sizeof(bitmap_entry);
    let cells = entry->cells;

    let cell_i = (ptr - (void *)region) / 16;
    for(let cell = cell_i; cell < cell_i + cells; cell++){
        let leaf_nr = cell / 64;

        let cell_in_leaf = cell % 64;

        // mark leaf cells as unused
        assert(bitmap_get_bit(cell_in_leaf, region->leafs[leaf_nr]) == 0);
        region->leafs[leaf_nr] |= 1llu << (63 - cell_in_leaf);
        assert(bitmap_get_bit(cell_in_leaf, region->leafs[leaf_nr]) == 1);

        
        region->root |= 1llu << (63 - leaf_nr);
    }



    region->allocs_count -= 1;


    // and maybe dealloc
    if(region->allocs_count == 2){
        // two allocations means chunk and region headers
        bitmap_region *prev = bitmap_first_region;
        while(prev->next != region){
            assert(prev->next != NULL);
            prev = prev->next;
        }

        assert(prev->next->next == region->next);
        prev->next = prev->next->next;
        free_chunk(chunk);
    }
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


// todo macro for constructor asserts
__attribute__((constructor)) void bitmap_init(){


    assert(!bitmap_get_bit(0, 4611686018427387904llu));   
    assert(bitmap_get_bit(1, 4611686018427387904llu));   
    assert(!bitmap_get_bit(2, 4611686018427387904llu));   
}