#define _GNU_SOURCE
#include "mem.h"

#include <stdbool.h>
#include <unistd.h>

#define noinline __attribute__((noinline))
#define inline __attribute__((always_inline))

#ifdef MEM_ALLOCATE_DEBUG_ENABLED
#define MEM_ALLOCATE_DEBUG(fn, msg) printf("%s: %s\n", fn, msg)
#else
#define MEM_ALLOCATE_DEBUG(fn, msg)
#endif

typedef enum {
	mem_type_none,
	Mem_Type_Stack,
	Mem_Type_Heap,
} Mem_Type;

typedef struct Mem_s {
	void *ptr;
	size_t size;
	Mem_Type type;
} Mem_s;

void *mem_ptr(Mem_s *mem) {
	if (mem == NULL) {
		return NULL;
	}
	return mem->ptr;
}

size_t mem_size(Mem_s *mem) {
	if (mem == NULL) {
		return 0;
	}
	return mem->size;
}

Mem_Type mem_type(Mem_s *mem) {
	if (mem == NULL) {
		return mem_type_none;
	}
	return mem->type;
}

#ifdef MEM_ALLOCATE_MMAP
#include <sys/mman.h>

#define MEM_ALLOCATE_FAILED MAP_FAILED

Mem_s *inline mem_alloc(size_t size_t) {
	if (size == 0) {
		return NULL;
	}

	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MEM_ALLOCATE_FAILED) {
		return MEM_ALLOCATE_FAILED;
	}

	Mem_s *mem = malloc(sizeof(Mem_s));
	mem->ptr = ptr;
	mem->size = size;
	mem->type = Heap;
	return mem;
}

Mem_s *inline mem_alloc_allow_stack(size_t size) {
	return mem_alloc(size);
}

int inline mem_free(Mem_s *mem, size_t size) {
	if (mem == NULL || mem->ptr == NULL) {
		return 1;
	}
	return munmap(mem->ptr, size);
}

#else

#include <pthread.h>
#include <stdint.h>  // for uintptr_t
#include <stdio.h>
#include <stdlib.h>  // for malloc, free

#define MEM_ALLOCATE_FAILED NULL

#define GET_RSP(rsp) __asm__("movq %%rsp, %0" : "=r"(rsp));

#define STACK_SAFE_MARGIN get_safe_stack_margin()

static long get_safe_stack_margin() {
	static long margin = 0;
	if (!margin) {
		margin = sysconf(_SC_PAGESIZE) * 2;
	}
	return margin;
}

ssize_t noinline get_stack_available(void) {
	pthread_attr_t attr;

	/*
	 * stack_addr is the lowest addressable byte of the stack.
	 *
	 * So the stack memory looks like this:
	 *
	 *   High Address
	 *   |------------------------| <- stack base
	 *   |------------------------|
	 *   |       stack top        | <- rsp (stack pointer, moves down)
	 *   |------------------------|
	 *   |      used stack        |
	 *   |------------------------|
	 *   |    available stack     |
	 *   |------------------------|
	 *   |       guard page       |
	 *   |------------------------|
	 *   |       stack base       | <- stack_addr (low address)
	 *   Low Address
	 *
	 * We compute:
	 *   available = (stack_addr + stack_size) - rsp
	 * which is:   = stack top - current stack pointer
	 *
	 * We subtract STACK_SAFE_MARGIN to avoid hitting the guard page.
	 */

	void *stack_addr;
	size_t stack_size;
	void *rsp;
	GET_RSP(rsp);

	pthread_getattr_np(pthread_self(), &attr);
	pthread_attr_getstack(&attr, &stack_addr, &stack_size);
	pthread_attr_destroy(&attr);

	printf("stack addr: %p; stack_size: %zu; rsp: %p\n", stack_addr, stack_size, rsp);

	ssize_t available = (uintptr_t)rsp - ((uintptr_t)stack_addr);
	return available - STACK_SAFE_MARGIN;
}

#define MEM_ALLOC_ALLOW_STACK(__mem_alloc_size, __mem_alloc_mem)                            \
	do {                                                                                    \
		if ((__mem_alloc_size) != 0 && (__mem_alloc_mem) != NULL) {                         \
			ssize_t __mem_alloc_stack_avail = get_stack_available();                        \
			if (__mem_alloc_stack_avail >= (ssize_t)((__mem_alloc_size) + sizeof(Mem_s))) { \
				MEM_ALLOCATE_DEBUG("mem_alloc_allow_stack", "allocating on stack");         \
				Mem_s __mem_alloc_stack_mem;                                                \
				__mem_alloc_stack_mem.ptr = alloca(__mem_alloc_size);                       \
				__mem_alloc_stack_mem.type = Mem_Type_Stack;                                \
				__mem_alloc_stack_mem.size = (__mem_alloc_size);                            \
				*(__mem_alloc_mem) = &__mem_alloc_stack_mem;                                \
			} else {                                                                        \
				MEM_ALLOCATE_DEBUG("mem_alloc_allow_stack", "allocating on heap");          \
				Mem_s *__mem_alloc_heap_mem = mem_alloc(__mem_alloc_size);                  \
				if (__mem_alloc_heap_mem != MEM_ALLOCATE_FAILED) {                          \
					*(__mem_alloc_mem) = __mem_alloc_heap_mem;                              \
				} else {                                                                    \
					mem_free(__mem_alloc_heap_mem);                                         \
					*(__mem_alloc_mem) = NULL;                                              \
				}                                                                           \
			}                                                                               \
			else {                                                                          \
				*(__mem_alloc_mem) = MEM_ALLOCATE_FAILED;                                   \
			}                                                                               \
		} else {                                                                            \
			if ((__mem_alloc_mem) != NULL) *(__mem_alloc_mem) = NULL;                       \
		}                                                                                   \
	} while (0)

Mem_s inline *mem_alloc(size_t size) {
	if (size == 0) {
		return MEM_ALLOCATE_FAILED;
	}

	MEM_ALLOCATE_DEBUG("mem_alloc", "allocating on heap");
	Mem_s *mem = malloc(sizeof(Mem_s));
	if (mem == NULL) {
		return MEM_ALLOCATE_FAILED;
	}

	void *ptr = malloc(size);
	if (ptr == NULL) {
		return MEM_ALLOCATE_FAILED;
	}

	mem->ptr = ptr;
	mem->size = size;
	mem->type = Mem_Type_Heap;
	return mem;
}

int inline mem_free(Mem_s *mem, size_t size) {
	int ret = 0;

	if (mem != NULL && mem->ptr != NULL && mem->type != Mem_Type_Stack) {
		free(mem->ptr);
	} else {
		ret = 1;
	}

	if (mem != NULL) {
		free(mem);
	} else {
		ret = 1;
	}

	return ret;
}

#endif

#include <stdio.h>

#define PRINT_RSP()                        \
	void *rsp;                             \
	__asm__("movq %%rsp, %0" : "=r"(rsp)); \
	printf("rsp: %p\n", rsp);

#define PRINT_STACK_BASE()                                  \
	pthread_attr_t attr;                                    \
	void *stack_addr;                                       \
	size_t stack_size;                                      \
	void *__rsp;                                            \
	GET_RSP(__rsp);                                         \
                                                            \
	pthread_getattr_np(pthread_self(), &attr);              \
	pthread_attr_getstack(&attr, &stack_addr, &stack_size); \
	pthread_attr_destroy(&attr);                            \
	printf("stack base: %p\n", stack_addr);

void test2() {
	printf("stack avail: %zu\n", get_stack_available());
	char a[1024];
}

void test() {
	printf("stack avail: %zu\n", get_stack_available());
	test2();
}

int main(void) {
	test();

	// //
	// void *ptr = mem_alloc_allow_stack(1024);
	// printf("stack avail after alloc: %zu\n", get_stack_available());
}