#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <unistd.h>

#include "mem/allocator.h"

typedef struct BGSlice_s BGSlice;

typedef struct BGSliceOption {
  size_t cap;
  size_t len;
  size_t elem_size;
  Allocator *allocator;
} BGSliceOption;

typedef bool (*slice_range_callback)(void *, size_t idx, void *ctx);

BGSlice *slice_new(size_t cap, size_t len, size_t elem_size);
BGSlice *slice_new_with_allocator(Allocator *allocator, size_t cap, size_t len,
                                  size_t elem_size);

BGSlice *slice_char_new(size_t cap, size_t len);
BGSlice *slice_reset(BGSlice *s);
BGSlice *slice_char_new_from_str(const char *str);
BGSlice *slice_new_from_slice(BGSlice *src, size_t start, size_t len);
ssize_t slice_get_len(BGSlice *s);
void slice_set_len(BGSlice *s, size_t len);
void slice_incr_len(BGSlice *s, size_t incr);
ssize_t slice_get_total_cap(BGSlice *s);
ssize_t slice_get_usable_cap(BGSlice *s);
void slice_free(BGSlice *s);
BGSlice *slice_grow(BGSlice *s);
BGSlice *slice_grow_to_cap(BGSlice *s, size_t cap);
void *slice_get_ptr_offset(BGSlice *s);
void *slice_get_ptr_begin_offset(BGSlice *s);
BGSlice *slice_append_n(BGSlice *s, void *const item, size_t n);
BGSlice *slice_append(BGSlice *s, void *const item);
bool slice_is_full(BGSlice *s);
ssize_t slice_copy(BGSlice *dst, BGSlice *src);
void slice_range(BGSlice *s, void *ctx, slice_range_callback callback);
void *slice_get(BGSlice *s, size_t index);

#endif  // SLICE_H