#include "bg_stack.h"

#include "bg_slice.h"
#include "mem/bg_allocator.h"

BGStack *
__BGStack_new(size_t len, size_t cap, size_t elem_size,
              struct BGStackOption *option)
{
    return __BGSlice_new(len, cap, elem_size,
                         (struct BGSliceOption *) option);
}

BGStack *
BGStack_push(BGStack *s, void *const item)
{
    return BGSlice_append(s, item);
}

size_t
BGStack_len(BGStack *s)
{
    return BGSlice_get_len(s);
}

void *
BGStack_pop(BGStack *s)
{
    size_t len = BGStack_len(s);
    if (len == 0)
        return NULL;
    void *item = BGSlice_get(s, len - 1);
    BGSlice_set_len(s, len - 1);
    return item;
}

void *
BGStack_peek(BGStack *s)
{
    size_t len = BGStack_len(s);
    if (len == 0)
        return NULL;
    return BGSlice_get(s, len - 1);
}

bool
BGStack_empty(BGStack *s)
{
    return BGStack_len(s) == 0;
}
