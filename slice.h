#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <unistd.h>

typedef struct Slice_s Slice;

Slice *slice_char_new(size_t cap, size_t len);
Slice *slice_reset(Slice *s);
Slice *slice_new(size_t cap, size_t len, size_t elem_size);
Slice *slice_char_new_from_str(const char *str);
Slice *slice_new_from_slice(Slice *src, size_t start, size_t len);
ssize_t slice_get_len(Slice *s);
void slice_set_len(Slice *s, size_t len);
void slice_incr_len(Slice *s, size_t incr);
ssize_t slice_get_total_cap(Slice *s);
ssize_t slice_get_usable_cap(Slice *s);
void slice_free(Slice *s);
Slice *slice_grow(Slice *s);
Slice *slice_grow_to_cap(Slice *s, size_t cap);
void *slice_get_ptr_offset(Slice *s);
Slice *slice_append(Slice *s, void *const item);
bool slice_is_full(Slice *s);
ssize_t slice_copy(Slice *dst, Slice *src);
void slice_range(Slice *s, bool (*callback)(void *, size_t idx, size_t len, size_t cap));
void *slice_get(Slice *s, size_t index);

#endif  // SLICE_H