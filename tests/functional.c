#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

/*
The plan:
- Allocate 20 GB of memory in total
- 1 GB allocated at average
- randomly choose posix_memalign, calloc, malloc, and check assumptions of used function
- write some random, deterministic data to each allocated block
- before destruction, check data
- sometimes use realloc (when?)

*/

void assert(bool a, char * msg){
    if(!a){
        printf("%s\n", msg);
        exit(1);
    }
}



void validate_rw_access(volatile char *ptr){
    // this may cause sigsegv, but nvm
    char tmp = *ptr;
    *ptr = 'Q';
    assert(*ptr == 'Q', "write to allocated block failed");
    *ptr = tmp;
}

void validate_malloc(void *ptr, size_t size){
    validate_rw_access((char *)ptr); // check if begining end of block is accessible
    validate_rw_access((char *)ptr + size - 1); // check if very end of block is accessible
}

void validate_zeroed(void *ptr, size_t size){
    for(size_t i = 0; i < size; i++){
        assert(*((char *)ptr + i) == 0, "calloc result not zeroed!");
    }
}

void validate_calloc(void *ptr, size_t count, size_t size){
    validate_malloc(ptr, count * size);
    validate_zeroed(ptr, count * size);
}

void *call_malloc(size_t size){
    void *res = malloc(size);
    assert(res != NULL, "");
    validate_malloc(res, size);
    return res;
}

void *call_calloc(size_t count, size_t size){
    void *res = calloc(count, size);
    validate_calloc(res, count, size);
    return res;
}

void validate_posix_memalign(int res, void **memptr, size_t alignment, size_t size){
    assert(res == 0, "couldn't posix_memalign()");
    assert((size_t)*memptr % alignment == 0, "posix_memalign() returned not aligned pointer");
    validate_malloc(*memptr, size);
}

int call_posix_memalign(void **memptr, size_t alignment, size_t size){
    int res = posix_memalign(memptr, alignment, size);
    validate_posix_memalign(res, memptr, alignment, size);
    return res;
}

void *call_realloc(void *ptr, size_t size){
    void * res = realloc(ptr, size);
    if(res != NULL){
        validate_malloc(res, size);
    }
    return res;
}

void call_free(void *ptr){
    assert(ptr != NULL, "trying to free(NULL)");
    free(ptr);
}


//////////////////////////////////////////////




typedef struct {
    void * ptr;
    uint64_t size;
    uint8_t seed;

} alloc;


#define _1GB 1073741824ull 
#define _100GB (20 * _1GB)
#define ALLOC_MAX _1GB
#define ALLOC_MIN 1
#define ALLOC_AVG 250000 //((ALLOC_MAX + ALLOC_MIN) / 2)
#define ALLOCS_MAX_NUM (_1GB / ALLOC_AVG)





uint64_t good_rand(){
    return    (((uint64_t) rand() <<  0) & 0x000000000000FFFFull) | 
              (((uint64_t) rand() << 16) & 0x00000000FFFF0000ull) | 
              (((uint64_t) rand() << 32) & 0x0000FFFF00000000ull) |
              (((uint64_t) rand() << 48) & 0xFFFF000000000000ull);
}
size_t rand_alloc_size(){
    float r = ((size_t)good_rand()) / (float)SIZE_MAX;//(good_rand() - ALLOC_MIN) % (ALLOC_MAX - ALLOC_MIN) + ALLOC_MIN;

    r = (2211222ull*r*r - 1304302ull*r + 47585ull) ;
    r = abs(r);
    return r+1;
}


alloc allocs[ALLOCS_MAX_NUM];
uint64_t sum_allocated_ever = 0;

void init(){
    for(size_t i = 0; i < ALLOCS_MAX_NUM; i++){
        allocs[i].size = 0;
    }
}

void check_and_free(alloc* a){
    uint8_t data = a->seed;
    for(uint8_t * i = a->ptr; i != (uint8_t *)a->ptr + a->size; i++){
        assert(*i == data, "data stored in memory was changed");
        data ++;
    }

    call_free(a->ptr);
    a->ptr = NULL;
    a->size = 0;
}

void fill_with_data(alloc* a){
    uint8_t data = a->seed;
    for(uint8_t * i = a->ptr; i != (uint8_t *)a->ptr + a->size; i++){
        *i = data;
        data ++;
    }
}

void allocate_here(alloc *a, size_t new_size){
    int r = rand() % 4;
    if(r == 0){
        // use malloc
        if(a->size != 0){
            check_and_free(a);
        }

        a->seed = good_rand();
        a->size = new_size;
        a->ptr = call_malloc(new_size);
    }else if(r == 1){
        // use posix_memalign
        if(a->size != 0){
            check_and_free(a);
        }
        size_t min_align = sizeof(void *);
        size_t align = min_align << rand() % 5;

        call_posix_memalign(&a->ptr, align, new_size);
        a->seed = good_rand();
        a->size = new_size;
    }else if(r == 2){
        // use calloc
        if(a->size != 0){
            check_and_free(a);
        }

        size_t size;
        for(int divisor = 32; divisor >= 1; divisor --){
            if(new_size % divisor == 0){
                size = divisor;
                break;
            } 
        }

        a->seed = good_rand();
        a->size = new_size;
        a->ptr = call_calloc(new_size / size, size);
    }else{
        // use realloc
        void * res = call_realloc(a->ptr, new_size);
        if(res != NULL){
            a->seed = good_rand();
            a->size = new_size;
            a->ptr = res;
        }
    }
}

void allocate_something(){
    alloc* a = &allocs[good_rand() % ALLOCS_MAX_NUM];

    size_t new_size = rand_alloc_size();

    allocate_here(a, new_size);

    sum_allocated_ever += new_size;

    fill_with_data(a);
}

void free_rest(){
    for(size_t i = 0; i < ALLOCS_MAX_NUM; i++){
        if(allocs[i].size != 0){
            check_and_free(&allocs[i]);
        }
    }
}

void test(){

    init();
    while(sum_allocated_ever < _100GB){
        allocate_something();
    }

    free_rest();  


}

int main(){
    srand(2);
    test();

}