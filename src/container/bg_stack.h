#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <unistd.h>

struct BGStackOption {
    struct Allocator *allocator;
};

typedef BGSlice BGStack;

BGStack *__BGStack_new(size_t len, size_t cap, size_t elem_size,
                       struct BGStackOption *option);
#define BGStack_new(type, len, cap, option) \
    __BGStack_new(len, cap, sizeof(type), option);

BGStack *BGStack_push(BGStack *s, void *const item);
size_t BGStack_len(BGStack *s);
void *BGStack_pop(BGStack *s);
void *BGStack_peek(BGStack *s);
bool BGStack_empty(BGStack *s);

#endif // STACK_H