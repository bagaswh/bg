#include "stack.h"

#include "slice.h"

typedef struct Slice_s Stack_s;

Stack *stack_create(size_t elem_size) {
	return slice_new(1, 0, elem_size);
}

Stack *stack_create_with_cap(size_t cap, size_t elem_size) {
	return slice_new(cap, 0, elem_size);
}

Stack *stack_create_with_len(size_t len, size_t elem_size) {
	return slice_new(1, len, elem_size);
}

Stack *stack_create_with_cap_and_len(size_t cap, size_t len, size_t elem_size) {
	return slice_new(cap, len, elem_size);
}

Stack *stack_push(Stack *s, void *item) {
	return slice_append(s, item);
}

size_t stack_len(Stack *s) {
	if (s == NULL) return 0;
	return slice_get_len(s);
}

void *stack_pop(Stack *s) {
	if (s == NULL) {
		return NULL;
	}
	size_t len = stack_len(s);
	void *item = slice_get(s, len);
}

bool stack_empty(Stack *s) {
	return slice_get_len(s) <= 0;
}
