#include <stdlib.h>

void *my_malloc(size_t size);
void *my_calloc(size_t count, size_t size);
int my_posix_memalign(void **memptr, size_t alignment, size_t size);