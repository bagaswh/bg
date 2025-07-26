#ifndef BG_SLICE_H
#define BG_SLICE_H

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include "bg_common.h"
#include "bg_types.h"
#include "mem/bg_allocator.h"

enum BGStatus {
    BG_OK,
    BG_ERR_ALLOC,
};

#define BG_SLICE_MAX_SIZE ((size_t) 0x10000000000)
#define BG_SIZE_AUTO (BG_SLICE_MAX_SIZE + (size_t) 1)

typedef struct BGSlice_s BGSlice;

struct BGSliceOption {
    struct Allocator *allocator;
};

struct BGSliceOption *BGSlice_with_allocator(struct BGSliceOption *option,
                                             struct Allocator *allocator);

BGSlice *__BGSlice_new(size_t len, size_t cap, size_t elem_size,
                       struct BGSliceOption *option);
#define BGSlice_new(type, len, cap, option) \
    __BGSlice_new(len, cap, sizeof(type), option)

BGSlice *__BGSlice_new_copy_from_buf(void *ptr, size_t size, size_t len,
                                     size_t cap, size_t elem_size,
                                     struct BGSliceOption *option);
#define BGSlice_new_copy_from_buf(buf, n, len, cap, option)                \
    __BGSlice_new_copy_from_buf((void *) buf, n, len, cap, sizeof(buf[0]), \
                                option)
#define BGSlice_new_copy_from_array(arr, len, cap, option)                  \
    __BGSlice_new_copy_from_buf((void *) arr, bg_arr_length(arr), len, cap, \
                                sizeof(arr[0]), option)

BGSlice *__BGSlice_new_from_buf(void *ptr, size_t len, size_t cap,
                                size_t elem_size,
                                struct BGSliceOption *option);
#define BGSlice_new_from_buf(buf, len, cap, option) \
    __BGSlice_new_from_buf((void *) buf, len, cap, sizeof(buf[0]), option)

BGSlice *BGSlice_new_from_slice(BGSlice *src, ssize_t low, ssize_t high,
                                struct BGSliceOption *option);

BGSlice *BGSlice_copy_from_slice(BGSlice *src, ssize_t low, ssize_t high,
                                 ssize_t cap, struct BGSliceOption *option);

BGSlice *BGSlice_reset(BGSlice *s);

ssize_t BGSlice_get_len(BGSlice *s);
void BGSlice_set_len(BGSlice *s, size_t len);
size_t BGSlice_get_cap(BGSlice *s);
size_t BGSlice_get_usable_cap(BGSlice *s);

void BGSlice_free(BGSlice *s);

void *BGSlice_get_data_ptr(BGSlice *s);
void *BGSlice_get_data_ptr_offset(BGSlice *s);

BGSlice *BGSlice_append(BGSlice *s, void *const item);
BGSlice *BGSlice_append_n(BGSlice *s, void *const item, size_t n);
bool BGSlice_is_full(BGSlice *s);
ssize_t BGSlice_copy(BGSlice *dst, BGSlice *src);
void *BGSlice_get(BGSlice *s, size_t index);
void *BGSlice_get_last(BGSlice *s);
void *BGSlice_set(BGSlice *s, size_t index, void *item);

typedef bool (*BGSlice_range_callback)(void *item, size_t idx, void *ctx);
void BGSlice_range(BGSlice *s, void *ctx, BGSlice_range_callback callback);

typedef i8 *comparable;
typedef ssize_t (*BGSlice_sort_comparator)(BGSlice *s, comparable a,
                                           comparable b, void *ctx);
typedef comparable (*BGSlice_sort_key_fn)(void *item, size_t idx);
enum BGStatus BGSlice_qsort(BGSlice *s, void *ctx, BGSlice_sort_key_fn key_fn,
                            BGSlice_sort_comparator comparator);

ssize_t BGSlice_default_comparator_asc(BGSlice *s, comparable a, comparable b,
                                       void *ctx);
ssize_t BGSlice_default_comparator_desc(BGSlice *s, comparable a,
                                        comparable b, void *ctx);
#endif // BG_SLICE_H