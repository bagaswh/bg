#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <unistd.h>

typedef struct Stack_s Stack;

Stack *stack_create(size_t elem_size);
Stack *stack_create_with_cap(size_t cap, size_t elem_size);
Stack *stack_create_with_len(size_t len, size_t elem_size);
Stack *stack_create_with_cap_and_len(size_t cap, size_t len, size_t elem_size);

Stack *stack_push(Stack *s, void *const item);
void *stack_pop(Stack *s);
void *stack_peek(Stack *s);
size_t stack_len(Stack *s);
bool stack_empty(Stack *s);

#endif  // STACK_H