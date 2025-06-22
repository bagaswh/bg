#ifndef MEM_H
#define MEM_H

#include <stdbool.h>
#include <unistd.h>

typedef struct Mem_s Mem;

Mem *mem_alloc(size_t size);

/*
 * Allocate a block of memory.
 *
 * If:
 * - the size is small enough
 * - allow_stack_alloc is true
 * - there's enough space in the stack
 * - not using mmap allocator
 * then the memory will be allocated on the stack.
 */
Mem *mem_alloc_allow_stack(size_t size);

/*
 * Frees a pointer allocated by mem_alloc.
 * The size parameter is required if using mmap allocator.
 */
int mem_free(Mem *mem, size_t size);

#endif /* MEM_H */