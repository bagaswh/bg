#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <unistd.h>

typedef struct Slice_s Slice;

Slice *slice_char_new(size_t cap, size_t len);
Slice *slice_new(size_t cap, size_t len, size_t elem_size);
ssize_t slice_get_len(Slice *s);
void *slice_set_len(Slice *s, size_t len);
void *slice_incr_len(Slice *s, size_t incr);
ssize_t slice_get_total_cap(Slice *s);
ssize_t slice_get_usable_cap(Slice *s);
void slice_free(Slice *s);
Slice *slice_grow(Slice *s);
Slice *slice_grow_to_cap(Slice *s, size_t cap);
void *slice_get_ptr_offset(Slice *s);
Slice *slice_append(Slice *s, void *const item);
bool slice_is_full(Slice *s);
ssize_t slice_copy(Slice *dst, Slice *src);

#endif  // SLICE_H