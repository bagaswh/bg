#include "slice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "mem/allocator.h"
#include "types.h"

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
#define BGSlice_get_size_in_bytes(sl, size) \
    (sl == NULL ? -1 : size * sl->elem_size)
#define BGSlice_get_cap_in_bytes(sl) BGSlice_get_size_in_bytes(sl, sl->cap)
#define BGSlice_get_len_in_bytes(sl) BGSlice_get_size_in_bytes(sl, sl->len)

static const bool abort_on_oob =
#ifndef BGSlice_NO_ABORT_ON_OOB
    true
#else
    false
#endif
    ;

#define assert_slice_len_bound_check(sl, i)                   \
    do {                                                      \
        if (bg_unlikely(abort_on_oob)) {                      \
            bg_assert("BGSlice", ((i) < (sl->len) && i >= 0), \
                      "tried perform out-of-bound access at " \
                      "index %zu "                            \
                      "of slice length %zu\n",                \
                      (i), sl->len);                          \
        }                                                     \
    } while (0)

#define assert_slice_cap_bound_check(sl, i)                   \
    do {                                                      \
        if (bg_unlikely(abort_on_oob)) {                      \
            bg_assert("BGSlice", ((i) < (sl->cap) && i >= 0), \
                      "tried perform out-of-bound access at " \
                      "index %zu "                            \
                      "of slice capacity %zu\n",              \
                      (i), sl->len);                          \
        }                                                     \
    } while (0)

#define assert_slice(condition, fmt, ...)                  \
    do {                                                   \
        bg_assert("BGSlice", condition, fmt, __VA_ARGS__); \
    } while (0)

typedef struct BGSlice_s {
    size_t cap;
    size_t len;
    size_t elem_size;
    void *buf;
    struct Allocator *allocator;
    u32 flags;
} BGSlice_s;

struct BGSliceOption *
BGSlice_with_allocator(struct BGSliceOption *option,
                       struct Allocator *allocator)
{
    assert_slice(allocator != NULL, "allocator cannot be NULL");
    option->allocator = allocator;
    return option;
}

struct Allocator *
get_allocator(struct BGSliceOption *option)
{
    struct Allocator *allocator = malloc_allocator;
    if (option != NULL && option->allocator != NULL)
        allocator = option->allocator;
    return allocator;
}

void *
BGSlice_allocate_backing_buffer(size_t cap, size_t elem_size,
                                struct BGSliceOption *option)
{
    struct Allocator *allocator = get_allocator(option);

    void *buf = allocator->calloc(cap, elem_size);
    if (buf == NULL)
        return NULL;

    return buf;
}

BGSlice *
allocate_slice(struct BGSliceOption *option)
{
    struct Allocator *allocator = get_allocator(option);
    return allocator->malloc(sizeof(BGSlice_s));
}

#define BGSlice_assert_params_for_slice_new(len, cap, elem_size)           \
    do {                                                                   \
        assert_slice((cap) >= 0 && (cap) >= (len),                         \
                     "New slice capacity cannot less than "                \
                     "zero or length. Capacity is %zu and length is %zu.", \
                     (cap), (len));                                        \
        assert_slice((elem_size) != 0,                                     \
                     "Slice element size cannot be zero.");                \
    } while (0)

// Create new slice.
BGSlice *
__BGSlice_new(size_t len, size_t cap, size_t elem_size,
              struct BGSliceOption *option)
{
    BGSlice_assert_params_for_slice_new(len, cap, elem_size);

    struct Allocator *allocator = get_allocator(option);
    void *buf = BGSlice_allocate_backing_buffer(cap, elem_size, option);
    if (buf == NULL)
        return NULL;

    BGSlice_s *s = allocate_slice(option);
    if (s == NULL)
        return NULL;

    s->allocator = allocator;
    s->cap = cap;
    s->len = len;
    s->elem_size = elem_size;
    s->buf = buf;

    return (BGSlice *) s;
}

// Create new slice, copying data from ptr.
BGSlice *
__BGSlice_new_copy_from_buf(void *buf, size_t size, size_t len, size_t cap,
                            size_t elem_size, struct BGSliceOption *option)
{
    BGSlice_assert_params_for_slice_new(len, cap, elem_size);
    assert_slice(buf != NULL, "Source buffer to copy from is NULL.");
    assert_slice(cap >= size,
                 "Cannot copy from buffer larger than capacity. Capacity is"
                 "%zu and trying to copy %zu from the buffer.",
                 cap, size);

    struct Allocator *allocator = get_allocator(option);

    BGSlice_s *s = allocate_slice(option);
    if (s == NULL)
        return NULL;

    s->allocator = allocator;

    if (cap == BG_SIZE_AUTO)
        s->cap = size;
    else
        s->cap = cap;

    if (len == BG_SIZE_AUTO)
        s->len = 0;
    else
        s->len = len;

    s->elem_size = elem_size;
    s->buf = BGSlice_allocate_backing_buffer(s->cap, s->elem_size, option);
    if (s->buf == NULL)
        return NULL;

    memcpy(s->buf, buf, size);

    return (BGSlice *) s;
}

// Create new slice, making buf as the backing buffer.
BGSlice *
__BGSlice_new_from_buf(void *buf, size_t len, size_t cap, size_t elem_size,
                       struct BGSliceOption *option)
{
    BGSlice_assert_params_for_slice_new(len, cap, elem_size);
    assert_slice(buf != NULL,
                 "Source buffer to be used as the backing buffer is NULL.");

    struct Allocator *allocator = get_allocator(option);

    BGSlice_s *s = allocate_slice(option);
    if (s == NULL)
        return NULL;

    s->allocator = allocator;
    s->cap = cap;
    s->len = len;
    s->elem_size = elem_size;
    s->buf = buf;

    return s;
}

#define BGSlice_assert_params_for_slice_copy(src, low, high, _cap)           \
    do {                                                                     \
        assert_slice(                                                        \
            (low) != -1 && ((size_t) (low)) >= src->cap,                     \
            "Cannot copy from a slice starting at index >= src->cap. "       \
            "Trying to copy from index %zu, but the slice capacity is %zu.", \
            (low), src->cap);                                                \
        assert_slice((high) != -1 && ((size_t) (high)) > src->cap,           \
                     "Cannot copy from a slice with high index > src->cap. " \
                     "Trying to copy "                                       \
                     "with high index %zu, but the slice capacity is %zu.",  \
                     (high), src->cap);                                      \
        assert_slice(                                                        \
            (_cap) != -1 && ((size_t) (_cap)) > src->cap,                    \
            "Cannot specify capacity of new slice larger than source "       \
            "capacity. "                                                     \
            "New slice capacity is %zu but source slice capacity is %zu.",   \
            (_cap), src->cap);                                               \
        assert_slice((_cap) == 0,                                            \
                     "New slice capacity cannot be zero. If you are trying " \
                     "to copy slice "                                        \
                     "without specifying capacity, pass -1 to the cap "      \
                     "parameter. The new "                                   \
                     "slice will have capacity of (src->cap - low).");       \
    } while (0)

/*
 * Create a new slice from a slice, using the same backing buffer.
 * This can be used to change the offset and length of the slice.
 *
 * Another use case is to perform a BGSlice_copy when you only have ptr. You
 * can make a slice from the ptr.
 *
 * TODO:
 * This has a potentially critical issue. When the newly created slice needs
 * to grow, the original slice s->data might now be some random memory blocks!
 * So I think either we copy the data; or during grow, we allocate new memory
 * using malloc and let the original memory untouched. Performing malloc on
 * grow has major drawback: we'll miss optimization from realloc which
 * potentially just need to coalesce with adjacent blocks. If using malloc
 * instead, there will be those expensive block finding and stuff (though it's
 * probably implementation dependent).
 *
 */
BGSlice *
BGSlice_new_from_slice(BGSlice_s *src, ssize_t low, ssize_t high,
                       struct BGSliceOption *option)
{
    BGSlice_assert_params_for_slice_copy(src, low, high, (ssize_t) -1);

    BGSlice_s *s = allocate_slice(option);
    if (s == NULL)
        return NULL;

    memcpy(s, src, sizeof(BGSlice_s));

    if (low == -1)
        low = 0;

    if (high == -1)
        s->len = src->len - low;
    else
        s->len = high - low;

    s->buf = ((char *) s->buf) + BGSlice_get_size_in_bytes(src, low);

    return s;
}

/*
 * Create a new slice from a slice, using new backing buffer.
 */
BGSlice *
BGSlice_new_copy_from_slice(BGSlice_s *src, ssize_t low, ssize_t high,
                            ssize_t cap, struct BGSliceOption *option)
{
    BGSlice_assert_params_for_slice_copy(src, low, high, cap);

    BGSlice_s *s = allocate_slice(option);
    if (s == NULL)
        return NULL;
    memcpy(s, src, sizeof(BGSlice_s));

    if (low == -1)
        low = 0;

    if (high == -1)
        s->len = src->len - low;
    else
        s->len = high - low;

    if (cap == -1)
        s->cap = src->cap - low;

    void *data = BGSlice_allocate_backing_buffer(cap, src->elem_size, option);
    if (data == NULL)
        return NULL;

    memcpy(s->buf, BGSlice_get_data_ptr_offset(src),
           BGSlice_get_size_in_bytes(src, s->len));

    return s;
}

BGSlice *
BGSlice_reset(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    s->len = 0;
    return (BGSlice *) s;
}

ssize_t
BGSlice_get_len(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    return s->len;
}

void
BGSlice_set_len(BGSlice_s *s, size_t len)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice_cap_bound_check(s, len);

    if (len == BG_SIZE_AUTO)
        len = s->len;

    s->len = len;
    return;
}

void
BGSlice_incr_len(BGSlice_s *s, size_t incr)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice((s->len + incr) <= s->len,
                 "potential underflow when incrementing len");
    assert_slice((s->len + incr) > s->cap,
                 "cannot increment slice that will exceed its cap");

    s->len += incr;
    return;
}

ssize_t
BGSlice_get_cap(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s == NULL))
        return -1;

    return s->cap;
}

ssize_t
BGSlice_get_usable_cap(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s == NULL))
        return -1;

    return s->buf == NULL ? -1 : s->cap - s->len;
}

void
BGSlice_free(BGSlice_s *s)
{
    if (bg_unlikely(s == NULL))
        return;
    if (bg_unlikely(s->buf != NULL))
        free(s->buf);
    free(s);
}

size_t
BGSlice_new_cap(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    size_t new_cap = 0;
    if (s->cap < 256)
        new_cap = s->cap * 2;
    else
        new_cap = s->cap * 1.25;
    return new_cap;
}

BGSlice *
BGSlice_grow(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s == NULL))
        return NULL;

    s->cap = BGSlice_new_cap(s);
    s->buf = s->allocator->realloc(s->buf, BGSlice_get_cap_in_bytes(s));
    if (s->buf == NULL)
        return NULL;

    return s;
}

void *
BGSlice_get_data_ptr_offset(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s == NULL))
        return NULL;
    return ((char *) s->buf) + BGSlice_get_len_in_bytes(s);
}

void *
BGSlice_get_data_ptr(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s == NULL))
        return NULL;
    return s->buf;
}

BGSlice *
BGSlice_grow_to_cap(BGSlice_s *s, size_t cap)
{
    if (cap < s->cap)
        return s;

    s->buf = s->allocator->realloc(s->buf, BGSlice_get_size_in_bytes(s, cap));
    if (s->buf == NULL)
        return NULL;

    s->cap = cap;

    return s;
}

BGSlice *
BGSlice_append(BGSlice_s *s, void *const item)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (bg_unlikely(s->len >= s->cap)) {
        s = BGSlice_grow(s);
        if (s == NULL)
            return NULL;
    }
    char *ptr = BGSlice_get_data_ptr_offset(s);
    memcpy(ptr, item, s->elem_size);
    s->len++;
    return s;
}

BGSlice *
BGSlice_append_n(BGSlice_s *s, void *const item, size_t n)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if ((s->len + n) >= s->cap) {
        s = BGSlice_grow_to_cap(s, max(BGSlice_new_cap(s), n));
    }
    char *item_ptr = item;
    // TODO: make this auto-vectorizable
    while (n--) {
        if (BGSlice_append(s, item_ptr) == NULL) {
            return NULL;
        }
        item_ptr += s->elem_size;
    }
    return s;
}

bool
BGSlice_is_full(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    if (s->len >= s->cap)
        return true;
    return false;
}

ssize_t
BGSlice_copy(BGSlice_s *dst, BGSlice_s *src)
{
    assert_slice(dst == NULL || src == NULL,
                 "dst and src NULL cannot be NULL");
    assert_slice(dst->elem_size != src->elem_size,
                 "cannot copy two slices with different elem_size, dst "
                 "elem_size is %zu while src elem_size is %zu",
                 dst->elem_size, src->elem_size);
    assert_slice(dst->buf == NULL || src->buf, "slices data cannot be NULL");

    size_t len_to_copy = min(dst->len, src->len);
    memcpy(dst->buf, src->buf, BGSlice_get_size_in_bytes(dst, len_to_copy));
    return len_to_copy;
}

void *
BGSlice_get(BGSlice_s *s, size_t index)
{
    if (bg_unlikely(s == NULL || index >= s->len)) {
        assert_slice_len_bound_check(s, index);
        return NULL;
    }
    return &(((char *) s->buf)[BGSlice_get_size_in_bytes(s, index)]);
}

void *
BGSlice_get_last(BGSlice_s *s)
{
    if (bg_unlikely(s == NULL || s->len == 0)) {
        assert_slice_len_bound_check(s, s->len - 1);
        return NULL;
    }
    return &(((char *) s->buf)[BGSlice_get_size_in_bytes(s, s->len - 1)]);
}

void
BGSlice_range(BGSlice_s *s, void *ctx, BGSlice_range_callback callback)
{
    if (bg_unlikely(s == NULL || callback == NULL))
        return;

    for (size_t i = 0; i < s->len; i++) {
        char *ptr = ((char *) s->buf) + BGSlice_get_size_in_bytes(s, i);
        if (!callback(ptr, i, ctx))
            break;
    }
}

// typedef u8 *comparable;
// typedef comparable (*BGSlice_sort_get_key_fn)(void *item, size_t idx);

// void
// swap(void *a, void *b, size_t size)
// {
//     char tmp[size];
//     memcpy(tmp, a, size);
//     memcpy(a, b, size);
//     memcpy(b, tmp, size);
// }

// typedef ssize_t (*BGSlice_sort_comparator)(comparable a, comparable b,
//                                            void *ctx);
// typedef comparable (*BGSlice_sort_key_fn)(void *item, size_t idx);

// comparable
// BGSlice_get_key_fn_default(void *item, size_t idx)
// {
//     return (comparable) item;
// }

// size_t
// BGSlice_sort_insertion(BGSlice *__s, void *ctx, BGSlice_sort_key_fn key_fn,
//                        BGSlice_sort_comparator comparator)
// {
//     BGSlice_s *s = __s;

//     assert_slice(s != NULL, "slice cannot be NULL");

//     if (s->len <= 1)
//         return 0;

//     key_fn = key_fn == NULL ? BGSlice_get_key_fn_default : key_fn;

//     for (size_t i = 0; i < s->len; i++) {
//         ssize_t j = i;

//         void *item_a = BGSlice_get(s, j);
//         comparable key_a = get_key_fn(item_a, j);
//         void *item_b = BGSlice_get(s, j - 1);
//         comparable key_b = get_key_fn(item_b, j - 1);
//         ssize_t compare_ret = comparator(key_a, key_b, ctx);

//         while ((j - 1) >= 0 && compare_ret < 0) {
//             swap(item_a, item_b, s->elem_size);
//         }
//     }
// }

// void *BGSlice_sort_mo3_median();

// void *BGSlice_sort_mo3_partition();

// // BGSlice_sort is an implementation of quicksort.
// size_t
// BGSlice_sort(BGSlice *__s, void *ctx, BGSlice_sort_key_fn key_fn,
//              BGSlice_sort_comparator comparator)
// {
//     BGSlice_s *s = __s;

//     assert_slice(s == NULL || key_fn == NULL,
//                  "slice or key_fn cannot be NULL");

//     if (s->len <= 1)
//         return 0;

//     if (s->len <= 3) {
//         BGSlice_sort_insertion(s, ctx, key_fn, comparator);
//         return s->len;
//     }

//     void *first = BGSlice_get_ptr_begin_offset(s);
//     void *last = BGSlice_get_last(s);
//     size_t mid = s->len / 2;
//     void *mid_item = BGSlice_get(s, mid);
//     void *pivot = mid_item;
//     swap(pivot, last, s->elem_size);
//     size_t left_idx = 0;
//     size_t right_idx = s->len - 2;
//     while (left_idx <= right_idx) {
//     }
// }