#ifndef BG_ALLOCATOR_H
#define BG_ALLOCATOR_H

#include <stdlib.h>
#include <unistd.h>

typedef struct Allocator {
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t size, size_t elem_size);
    void *(*realloc)(void *ptr, size_t size);
    void *(*aligned_alloc)(size_t alignment, size_t size);
    void (*free)(void *ptr);
} Allocator;

static Allocator *malloc_allocator = &(struct Allocator) {
    .malloc = malloc,
    .calloc = calloc,
    .realloc = realloc,
    .aligned_alloc = aligned_alloc,
    .free = free,
};

#endif // BG_ALLOCATOR_H