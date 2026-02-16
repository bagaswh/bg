#include "bg_slice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bg_common.h"
#include "bg_types.h"
#include "math/bg_math.h"
#include "mem/bg_allocator.h"

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
#define BGSlice_get_size_in_bytes(sl, size) (size * sl->elem_size)
#define BGSlice_get_cap_in_bytes(sl) BGSlice_get_size_in_bytes(sl, sl->cap)
#define BGSlice_get_len_in_bytes(sl) BGSlice_get_size_in_bytes(sl, sl->len)

#ifndef assert_slice_len_bound_check
#    ifndef BG_SLICE_NO_ABORT_ON_OOB
#        define assert_slice_len_bound_check(sl, i)                 \
            do {                                                    \
                bg_assert("BGSlice", ((i) < (sl->len) && (i) >= 0), \
                          "tried perform out-of-bound access at "   \
                          "index %zu "                              \
                          "of slice length %zu\n",                  \
                          (i), sl->len);                            \
            } while (0)

#        define assert_slice_cap_bound_check(sl, i)                 \
            do {                                                    \
                bg_assert("BGSlice", ((i) < (sl->cap) && (i) >= 0), \
                          "tried perform out-of-bound access at "   \
                          "index %zu "                              \
                          "of slice capacity %zu\n",                \
                          (i), sl->len);                            \
            } while (0)
#    else
#        define assert_slice_len_bound_check(sl, i)
#        define assert_slice_cap_bound_check(sl, i)
#    endif

#endif

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

    if (option == NULL) {
        option = malloc(sizeof(struct BGSliceOption));
        if (option == NULL)
            return NULL;
    }

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

    memcpy(s->buf, buf, size * s->elem_size);

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
    bg_assert(
        "BGSlice", ((len) <= (s->cap) && (len) >= 0),
        "Tried to set slice length to %zu, but the slice capacity is %zu.",
        (len), s->cap);

    if (len == BG_SIZE_AUTO)
        len = s->len;

    s->len = len;
    return;
}

void
BGSlice_incr_len(BGSlice_s *s, size_t incr)
{
    if (bg_unlikely(incr == 0))
        return;

    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice(
        (s->len + incr) >= s->len,
        "Potential underflow when incrementing length. Length is %zu, "
        "after incrementing by %zu it's %zu.",
        s->len, incr, (s->len + incr));
    assert_slice(
        (s->len + incr) <= s->cap,
        "Cannot increment length that will exceed slice capacity. Length is "
        "%zu, capacity is %zu, after incrementing length by %zu it's %zu.",
        s->len, s->cap, incr, (s->len + incr));

    s->len += incr;
    return;
}

size_t
BGSlice_get_cap(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    return s->cap;
}

size_t
BGSlice_get_usable_cap(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice(s->buf != NULL, "slice buffer cannot be NULL");

    return s->cap - s->len;
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
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice(cap > s->cap, "cannot grow slice to smaller capacity");

    if (cap <= s->cap)
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
    assert_slice(dst != NULL || src != NULL,
                 "dst and src NULL cannot be NULL");
    assert_slice(dst->elem_size == src->elem_size,
                 "cannot copy two slices with different elem_size, dst "
                 "elem_size is %zu while src elem_size is %zu",
                 dst->elem_size, src->elem_size);
    assert_slice(dst->buf != NULL || src->buf != NULL,
                 "slices data cannot be NULL");

    size_t len_to_copy = min(dst->len, src->len);
    memcpy(dst->buf, src->buf, BGSlice_get_size_in_bytes(dst, len_to_copy));
    return len_to_copy;
}

void *BGSlice_get(BGSlice_s *s, size_t index);
void BG_INLINE *__BGSlice_get(BGSlice_s *s, size_t index);
void *BGSlice_set(BGSlice_s *s, size_t index, void *item);
void BG_INLINE *__BGSlice_set(BGSlice_s *s, size_t index, void *item);

void *
BGSlice_get(BGSlice_s *s, size_t index)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice_len_bound_check(s, index);

    return __BGSlice_get(s, index);
}

void BG_INLINE *
__BGSlice_get(BGSlice_s *s, size_t index)
{
    return &(((char *) s->buf)[BGSlice_get_size_in_bytes(s, index)]);
}

void BG_INLINE *
__BGSlice_set(BGSlice_s *s, size_t index, void *item)
{
    memcpy(s->buf + BGSlice_get_size_in_bytes(s, index), item, s->elem_size);
    return item;
}

void *
BGSlice_set(BGSlice_s *s, size_t index, void *item)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice_len_bound_check(s, index);

    return __BGSlice_set(s, index, item);
}

void *
BGSlice_get_last(BGSlice_s *s)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice_len_bound_check(s, s->len - 1);

    return __BGSlice_get(s, s->len - 1);
}

void
BGSlice_range(BGSlice_s *s, void *ctx, BGSlice_range_callback callback)
{
    assert_slice(s != NULL, "slice cannot be NULL");
    assert_slice(callback != NULL, "callback cannot be NULL");

    for (size_t i = 0; i < s->len; i++) {
        char *ptr = __BGSlice_get(s, i);
        if (!callback(ptr, i, ctx))
            break;
    }
}

#define BG_SLICE_ITERATOR_FLAG_NO_BOUND_CHECK (1 << 0)

////////////////////
// Iterator
//
// struct BGSliceIterator {
//     BGSlice *slice;
//     size_t idx;
//     void *(*next)(struct BGSliceIterator *iter);
// };

// struct BGSliceIterator
// BGSlice_iter(BGSlice *slice, size_t hint_low, size_t hint_max)
// {
//     struct BGSliceIterator iter = { .slice = slice, .idx = 0 };

//     u32 flags = 0;
//     if (hint_low >= 0 && hint_max < slice->len)
//         iter.next = BGSlice_iter_next_no_bound_check;
//     else
//         iter.next = BGSlice_iter_next_bound_checked;

//     return iter;
// }

// void *
// BGSlice_iter_next_bound_checked(struct BGSliceIterator *iter)
// {
//     void *item = BGSlice_get(iter->slice, iter->idx);
//     iter->idx++;
//     return item;
// }

// void *
// BGSlice_iter_next_no_bound_check(struct BGSliceIterator *iter)
// {
//     void *item = __BGSlice_get(iter->slice, iter->idx);
//     iter->idx++;
//     return item;
// }

#ifdef BG_SLICE_EXAMPLE
int
main(void)
{
    int data[] = { 5, 1, 4, 1, 6, 7, 9, 2, 3, 5 };
    BGSlice *slice =
        BGSlice_new_copy_all_from_array(data, BG_AUTO, BG_AUTO, NULL);
    struct BGSliceIterator iter = BGSlice_iter(slice, 0, `);
}
#endif

////////////////////
// Searching
//

/**
 * Search item matching `term` in the slice using binary search, return the
 * index. Needs to be sorted first.
 */
// ssize_t
// BGSlice_binary_search_indexof(BGSlice *s, void *term)
// {
//     if (s->len <= 10)
//         return BGSlice_linear_search_indexof(s, term);

//     size_t low = 0;
//     size_t high = s->len;
//     size_t mid = (high - low) / 2;
//     while (true) {
//         void *item = __BGSlice_get(s, mid);
//         int cmp_ret = memcmp(item, term, s->elem_size);
//         if (cmp_ret == 0)
//             return mid;
//         else if (cmp_ret < 0)
//             high = mid - 1;
//         else if (cmp_ret > 0)
//             low = mid + 1;
//     }
// }

// ssize_t
// BGSlice_linear_search_indexof(BGSlice *s, void *term)
// {
// }

////////////////////
// Sorting
//

comparable BG_INLINE
BGSlice_key_fn_default(void *item, size_t idx)
{
    return (comparable) item;
}

ssize_t
BGSlice_default_comparator_asc(BGSlice_s *s, comparable a, comparable b,
                               void *ctx)
{
    return memcmp(a, b, s->elem_size);
}

ssize_t
BGSlice_default_comparator_desc(BGSlice_s *s, comparable a, comparable b,
                                void *ctx)
{
    return -memcmp(a, b, s->elem_size);
}

enum BGStatus
BGSlice_isort(BGSlice_s *s, void *ctx, BGSlice_sort_key_fn key_fn,
              BGSlice_sort_comparator comparator)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    if (s->len <= 1)
        return 0;

    key_fn = key_fn == NULL ? BGSlice_key_fn_default : key_fn;
    comparator =
        comparator == NULL ? BGSlice_default_comparator_asc : comparator;

    void *swap_tmp = s->allocator->malloc(s->elem_size);
    if (swap_tmp == NULL)
        return BG_ERR_ALLOC;

    for (size_t i = 1; i < s->len; i++) {
        ssize_t j = i;

        while ((j - 1) >= 0) {
            void *item_a = BGSlice_get(s, j);
            comparable key_a = key_fn(item_a, j);

            void *item_b = BGSlice_get(s, j - 1);
            comparable key_b = key_fn(item_b, j - 1);

            ssize_t compare_ret = comparator(s, key_b, key_a, ctx);
            if (compare_ret > 0)
                bg_swap_generic(item_a, item_b, swap_tmp, s->elem_size);
            j--;
        }
    }

    return BG_OK;
}

bool
print_slice_item(void *item, size_t idx, void *ctx)
{
    int *value = (int *) item;
    printf("%d ", *value);
    return true;
}

struct pivot_buf_t {
    void *value;
    size_t index;
};

struct qsort_mo3_pick_median_args {
    BGSlice_s *s;
    BGSlice_s *pivot_indices;
    size_t low;
    size_t high;
    size_t median_idx_result;
};

void
BGSlice_qsort_mo3_pick_median(struct qsort_mo3_pick_median_args *args)
{
    size_t space_size = args->high - args->low;

    struct fastrand rng = bg_fast_rand_init();

    for (size_t i = 0; i < 3; i++) {
        size_t idx = bg_fast_rand_n(&rng, space_size) + args->low;
        for (ssize_t j = i - 1; j > 0; j--) {
            while (idx == *(size_t *) BGSlice_get(args->pivot_indices, j)) {
                idx = bg_fast_rand_n(&rng, space_size) + args->low;
            }
        }
        BGSlice_set(args->pivot_indices, i, &idx);
    }

    BGSlice_isort(args->pivot_indices, NULL, NULL, NULL);
    args->median_idx_result = *(size_t *) BGSlice_get(args->pivot_indices, 1);
}

struct qsort_mo3_partition_args {
    BGSlice_s *s;
    size_t low;
    size_t high;
    void *swap_tmp;
    size_t median_idx_result;
    BGSlice *pivot_indices;
};

void
BGSlice_qsort_mo3_partition(struct qsort_mo3_partition_args *args)
{
    struct qsort_mo3_pick_median_args margs = {
        .s = args->s,
        .pivot_indices = args->pivot_indices,
        .low = args->low,
        .high = args->high,
    };

    BGSlice_qsort_mo3_pick_median(&margs);

    size_t median_idx = margs.median_idx_result;

    // printf("median_idx=%zu\n", median_idx);

    bg_swap_generic(BGSlice_get(args->s, median_idx),
                    BGSlice_get(args->s, args->high - 1), args->swap_tmp,
                    args->s->elem_size);

    void *pivot = BGSlice_get(args->s, args->high - 1);
    ssize_t i = args->low;
    ssize_t j = args->high - 2;

    while (1) {
        while (1) {
            if (memcmp(BGSlice_get(args->s, i), pivot, args->s->elem_size)
                >= 0)
                break;
            if (i < args->high - 1)
                i++;
            else
                break;
        }
        while (1) {
            if (memcmp(BGSlice_get(args->s, j), pivot, args->s->elem_size)
                <= 0)
                break;
            if (j > 0)
                j--;
            else
                break;
        }
        if (i >= j)
            break;

        bg_swap_generic(BGSlice_get(args->s, i), BGSlice_get(args->s, j),
                        args->swap_tmp, args->s->elem_size);
    }
    bg_swap_generic(BGSlice_get(args->s, i), pivot, args->swap_tmp,
                    args->s->elem_size);
    args->median_idx_result = i;
}

struct __qsort_args {
    BGSlice_s *s;
    void *ctx;
    BGSlice_sort_key_fn key_fn;
    BGSlice_sort_comparator comparator;
    size_t low;
    size_t high;
    void *swap_tmp;
    BGSlice *pivot_indices;
};

void
__BGSlice_qsort(struct __qsort_args *args)
{
    struct qsort_mo3_partition_args partition_args = {
        .s = args->s,
        .low = args->low,
        .high = args->high,
        .swap_tmp = args->swap_tmp,
        .pivot_indices = args->pivot_indices,
    };

    BGSlice_qsort_mo3_partition(&partition_args);

    size_t pivot_idx = partition_args.median_idx_result;

    args->low = args->low;
    args->high = pivot_idx - 1;
    __BGSlice_qsort(args);

    args->low = pivot_idx + 1;
    args->high = args->high;
    __BGSlice_qsort(args);
}

// BGSlice_qsort is an implementation of quicksort.
enum BGStatus
BGSlice_qsort(BGSlice_s *s, void *ctx, BGSlice_sort_key_fn key_fn,
              BGSlice_sort_comparator comparator)
{
    assert_slice(s != NULL, "slice cannot be NULL");

    enum BGStatus ret = BG_OK;

    if (s->len <= 1)
        goto done;

    key_fn = key_fn == NULL ? BGSlice_key_fn_default : key_fn;
    comparator =
        comparator == NULL ? BGSlice_default_comparator_asc : comparator;

    if (s->len <= 15) {
        BGSlice_isort(s, ctx, key_fn, comparator);
        goto done;
    }

    void *swap_tmp = s->allocator->malloc(s->elem_size);
    if (swap_tmp == NULL) {
        ret = BG_ERR_ALLOC;
        goto free_swap_tmp;
    }

    BGSlice *pivot_indices = __BGSlice_new(
        3, 3, sizeof(size_t), BGSlice_with_allocator(NULL, s->allocator));
    if (pivot_indices == NULL) {
        ret = BG_ERR_ALLOC;
        goto free_pivot_indices;
    }

    struct __qsort_args args = {
        .s = s,
        .ctx = ctx,
        .key_fn = key_fn,
        .comparator = comparator,
        .low = 0,
        .high = s->len,
        .swap_tmp = swap_tmp,
        .pivot_indices = pivot_indices,
    };

    __BGSlice_qsort(&args);

free_pivot_indices:
    s->allocator->free(pivot_indices);
free_swap_tmp:
    s->allocator->free(swap_tmp);

done:
    return ret;
}