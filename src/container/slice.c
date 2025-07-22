#include "slice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "common.h"
#include "types.h"

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
#define bg_slice_get_size_in_bytes(sl, size) \
    (sl == NULL ? -1 : size * sl->elem_size)
#define bg_slice_get_cap_in_bytes(sl) bg_slice_get_size_in_bytes(sl, sl->cap)
#define bg_slice_get_len_in_bytes(sl) bg_slice_get_size_in_bytes(sl, sl->len)

static const bool abort_on_oob =
#ifndef BG_SLICE_NO_ABORT_ON_OOB
    true
#else
    false
#endif
    ;

#define print_backtrace()                                                  \
    do {                                                                   \
        if (unlikely(print_stacktrace_before_abort)) {                     \
            void *bt[32];                                                  \
            int bt_size = backtrace(bt, 32);                               \
            char **bt_syms = backtrace_symbols(bt, bt_size);               \
            if (bt_syms) {                                                 \
                printf("<bg_slice>: Stack trace:\n");                      \
                for (int i = 0; i < bt_size; ++i) {                        \
                    printf("  [%d] %s\n", i, bt_syms[i]);                  \
                }                                                          \
                free(bt_syms);                                             \
            } else {                                                       \
                printf("<bg_slice>: Failed to get stack trace symbols\n"); \
            }                                                              \
        }                                                                  \
    } while (0)

#define assert_slice_bound_check(sl, i)                                    \
    do {                                                                   \
        if (unlikely(abort_on_oob && (i >= sl->len || i < 0))) {           \
            printf(                                                        \
                "<bg_slice>: %s:%d: tried perform out-of-bound access at " \
                "index %d "                                                \
                "of slice length %d\n",                                    \
                __FILE__, __LINE__, i, sl->len);                           \
            print_backtrace();                                             \
            abort();                                                       \
        }                                                                  \
    } while (0)

typedef struct bg_slice {
    size_t cap;
    size_t len;
    size_t elem_size;
    void *data;
    Allocator *allocator;
} bg_slice;

struct bg_slice *
bg_slice_new_with_allocator(Allocator *allocator, size_t cap, size_t len,
                            size_t elem_size)
{
    if (cap == 0 || elem_size == 0) {
        return NULL;
    }

    void *data = allocator->calloc(cap, elem_size);
    if (data == NULL) {
        return NULL;
    }

    struct bg_slice *s = allocator->malloc(sizeof(struct bg_slice));
    if (s == NULL) {
        return NULL;
    }
    s->allocator = allocator;
    s->cap = cap;
    s->len = len;
    s->elem_size = elem_size;
    s->data = data;

    return s;
}

struct bg_slice *
bg_slice_copy_from_ptr_with_allocator(Allocator *allocator, void *ptr,
                                      size_t cap, size_t len,
                                      size_t elem_size)
{
    if (cap == 0 || elem_size == 0) {
        return NULL;
    }

    void *data = allocator->calloc(cap, elem_size);
    if (data == NULL) {
        return NULL;
    }

    struct bg_slice *s = allocator->malloc(sizeof(struct bg_slice));
    if (s == NULL) {
        return NULL;
    }
    s->allocator = allocator;
    s->cap = cap;
    s->len = len;
    s->elem_size = elem_size;
    s->data = data;
    memcpy(s->data, ptr, bg_slice_get_len_in_bytes(s));

    return s;
}

struct bg_slice *
bg_slice_new_from_ptr_with_allocator(Allocator *allocator, void *ptr,
                                     size_t cap, size_t len, size_t elem_size)
{
    if (cap == 0 || elem_size == 0) {
        return NULL;
    }

    struct bg_slice *s = allocator->malloc(sizeof(struct bg_slice));
    if (s == NULL) {
        return NULL;
    }
    s->allocator = allocator;
    s->cap = cap;
    s->len = len;
    s->elem_size = elem_size;
    s->data = ptr;

    return s;
}

struct bg_slice *
bg_slice_char_new_with_allocator(Allocator *allocator, size_t cap, size_t len)
{
    return bg_slice_new_with_allocator(allocator, cap, len, sizeof(char));
}

struct bg_slice *
bg_slice_char_new_from_str_with_allocator(Allocator *allocator,
                                          const char *str)
{
    return bg_slice_copy_from_ptr_with_allocator(
        allocator, str, strlen(str) + 1, strlen(str) + 1, sizeof(char));
}

/*
 * Create a new slice from a slice, using the same backing buffer.
 * This can be used to change the offset and length of the slice.
 *
 * Another use case is to perform a bg_slice_copy when you only have ptr. You
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
 * implementation dependent).
 *
 */
struct bg_slice *
bg_slice_new_from_bg_slice_with_allocator(Allocator *allocator,
                                          struct bg_slice *src, size_t start,
                                          ssize_t len)
{
    if (start >= src->cap || len > src->cap) {
        return NULL;
    }

    struct bg_slice *s = allocator->malloc(sizeof(struct bg_slice));
    if (s == NULL) {
        return NULL;
    }

    memcpy(s, src, sizeof(struct bg_slice));

    if (len == -1) {
        s->len = src->len - start;
    } else {
        s->len = len - start;
    }

    s->data = src->data + bg_slice_get_size_in_bytes(src, start);

    return s;
}

/*
 * Create a new slice from a slice, using new backing buffer.
 */
struct bg_slice *
bg_slice_copy_from_bg_slice_with_allocator(Allocator *allocator,
                                           struct bg_slice *src, size_t start,
                                           ssize_t len, ssize_t cap)
{
    if (start >= src->cap || len > src->cap) {
        return NULL;
    }

    struct bg_slice *s = allocator->malloc(sizeof(struct bg_slice));
    if (s == NULL) {
        return NULL;
    }

    memcpy(s, src, sizeof(struct bg_slice));

    if (len == -1) {
        s->len = src->len - start;
    } else {
        s->len = len - start;
    }

    if (cap == -1) {
        s->cap = src->cap - start;
    } else {
        s->cap = cap - start;
    }

    s->data = allocator->malloc(s->cap);
    if (s->data == NULL) {
        return NULL;
    }

    memcpy(s->data, src->data + bg_slice_get_size_in_bytes(src, start),
           bg_slice_get_size_in_bytes(src, s->len));

    return s;
}

struct bg_slice *
bg_slice_new_from_slice(struct bg_slice *src, size_t start, ssize_t len)
{
    return bg_slice_new_from_bg_slice_with_allocator(&Malloc_Allocator, src,
                                                     start, len);
}

struct bg_slice *
bg_slice_copy_from_slice(Allocator *allocator, struct bg_slice *src,
                         size_t start, ssize_t len, ssize_t cap)
{
    return bg_slice_copy_from_bg_slice_with_allocator(&Malloc_Allocator, src,
                                                      start, len, cap);
}

struct bg_slice *
bg_slice_new(size_t cap, size_t len, size_t elem_size)
{
    return bg_slice_new_with_allocator(&Malloc_Allocator, cap, len,
                                       elem_size);
}

struct bg_slice *
bg_slice_char_new(size_t cap, size_t len)
{
    return bg_slice_char_new_with_allocator(&Malloc_Allocator, cap, len);
}

struct bg_slice *
bg_slice_char_new_from_str(const char *str)
{
    return bg_slice_char_new_from_str_with_allocator(&Malloc_Allocator, str);
}

struct bg_slice *
bg_slice_reset(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return NULL;
    }
    s->len = 0;
    return s;
}

ssize_t
bg_slice_get_len(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return -1;
    }
    return s->len;
}

void
bg_slice_set_len(struct bg_slice *s, size_t len)
{
    if (unlikely(s == NULL)) {
        return;
    }
    if (len > s->cap || len < 0) {
        return;
    }
    s->len = len;
    return;
}

void
bg_slice_incr_len(struct bg_slice *s, size_t incr)
{
    if (unlikely(s == NULL)) {
        return;
    }
    if (unlikely((s->len + incr) > s->cap || (s->len + incr) < 0)) {
        return;
    }
    s->len += incr;
    return;
}

ssize_t
bg_slice_get_cap(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return -1;
    }
    return s->cap;
}

ssize_t
bg_slice_get_usable_cap(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return -1;
    }
    return s->data == NULL ? -1 : s->cap - s->len;
}

void
bg_slice_free(struct bg_slice *s)
{
    if (unlikely(s != NULL && s->data != NULL)) {
        free(s->data);
    }

    if (unlikely(s != NULL)) {
        free(s);
    }
}

size_t
bg_slice_new_cap(struct bg_slice *s)
{
    size_t new_cap = 0;
    if (s->cap < 256) {
        new_cap = s->cap * 2;
    } else {
        new_cap = s->cap * 1.25;
    }
    return new_cap;
}

struct bg_slice *
bg_slice_grow(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return NULL;
    }
    s->cap = bg_slice_new_cap(s);
    s->data = s->allocator->realloc(s->data, bg_slice_get_cap_in_bytes(s));
    if (s->data == NULL) {
        return NULL;
    }
    return s;
}

void *
bg_slice_get_ptr_offset(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return NULL;
    }
    return s->data + bg_slice_get_len_in_bytes(s);
}

void *
bg_slice_get_ptr_begin_offset(struct bg_slice *s)
{
    if (unlikely(s == NULL)) {
        return NULL;
    }
    return s->data;
}

struct bg_slice *
bg_slice_grow_to_cap(struct bg_slice *s, size_t cap)
{
    if (cap < s->cap) {
        return s;
    }
    s->data =
        s->allocator->realloc(s->data, bg_slice_get_size_in_bytes(s, cap));
    s->cap = cap;
    if (s->data == NULL) {
        return NULL;
    }
    return s;
}

struct bg_slice *
bg_slice_append(struct bg_slice *s, void *const item)
{
    if (unlikely(s == NULL)) {
    }
    if (unlikely(s->len >= s->cap)) {
        s = bg_slice_grow(s);
        if (s == NULL) {
            return NULL;
        }
    }
    char *ptr = bg_slice_get_ptr_offset(s);
    memcpy(ptr, item, s->elem_size);
    s->len++;
    return s;
}

struct bg_slice *
bg_slice_append_n(struct bg_slice *s, void *const item, size_t n)
{
    if ((s->len + n) >= s->cap) {
        s = bg_slice_grow_to_cap(s, max(bg_slice_new_cap(s), n));
    }
    char *item_ptr = item;
    // TODO: make this auto-vectorizable
    while (n--) {
        if (bg_slice_append(s, item_ptr) == NULL) {
            return NULL;
        }
        item_ptr += s->elem_size;
    }
    return s;
}

bool
bg_slice_is_full(struct bg_slice *s)
{
    if (s != NULL && s->len >= s->cap) {
        return true;
    }
    return false;
}

ssize_t
bg_slice_copy(struct bg_slice *dst, struct bg_slice *src)
{
    if (dst == NULL || src == NULL) {
        return -1;
    }
    if (dst->elem_size != src->elem_size) {
        return -1;
    }
    if (dst->data == NULL || src->data == NULL) {
        return -1;
    }
    size_t len_to_copy = min(dst->len, src->len);
    memcpy(dst->data, src->data,
           bg_slice_get_size_in_bytes(dst, len_to_copy));
    return len_to_copy;
}

void *
bg_slice_get(struct bg_slice *s, size_t index)
{
    if (unlikely(s == NULL || index >= s->len)) {
        assert_slice_bound_check(sl, index);
        return NULL;
    }
    return &(((char *) s->data)[bg_slice_get_size_in_bytes(s, index)]);
}

void *
bg_slice_get_last(struct bg_slice *s)
{
    if (unlikely(s == NULL || s->len == 0)) {
        assert_slice_bound_check(sl, index);
        return NULL;
    }
    return &(((char *) s->data)[bg_slice_get_size_in_bytes(s, s->len - 1)]);
}

void
bg_slice_range(struct bg_slice *s, void *ctx,
               bg_slice_range_callback callback)
{
    if (unlikely(s == NULL || callback == NULL)) {
        return;
    }
    for (size_t i = 0; i < s->len; i++) {
        char *ptr = s->data + bg_slice_get_size_in_bytes(s, i);
        if (!callback(ptr, i, ctx)) {
            break;
        }
    }
}

typedef u8 *comparable;
typedef comparable (*bg_slice_sort_get_key_fn)(void *item, size_t idx);

void
swap(void *a, void *b, size_t size)
{
    char tmp[size];
    memcpy(tmp, a, size);
    memcpy(a, b, size);
    memcpy(b, tmp, size);
}

typedef ssize_t (*bg_slice_sort_comparator)(comparable a, comparable b,
                                            void *ctx);
typedef comparable (*bg_slice_sort_key_fn)(void *item, size_t idx);

comparable
bg_slice_get_key_fn_default(void *item, size_t idx)
{
    return (comparable) item;
}

size_t
bg_slice_sort_insertion(struct bg_slice *s, void *ctx,
                        bg_slice_sort_key_fn key_fn,
                        bg_slice_sort_comparator comparator)
{
    if (s == NULL || s->len <= 1) {
        return 0;
    }

    key_fn = key_fn == NULL ? bg_slice_get_key_fn_default : key_fn;

    for (size_t i = 0; i < s->len; i++) {
        ssize_t j = i;

        void *item_a = bg_slice_get(s, j);
        comparable key_a = get_key_fn(item_a, j);
        void *item_b = bg_slice_get(s, j - 1);
        comparable key_b = get_key_fn(item_b, j - 1);
        ssize_t compare_ret = comparator(key_a, key_b, ctx);

        while ((j - 1) >= 0 && compare_ret < 0) {
            swap(item_a, item_b, s->elem_size);
        }
    }
}

size_t bg_slice_sort_sse4(struct bg_slice *s, void *ctx,
                          bg_slice_sort_key_fn get_key_fn,
                          bg_slice_sort_comparator comparator);
size_t bg_slice_sort_avx(struct bg_slice *s, void *ctx,
                         bg_slice_sort_key_fn get_key_fn,
                         bg_slice_sort_comparator comparator);
size_t bg_slice_sort_avx2(struct bg_slice *s, void *ctx,
                          bg_slice_sort_key_fn get_key_fn,
                          bg_slice_sort_comparator comparator);

void *bg_slice_sort_mo3_median();

void *bg_slice_sort_mo3_partition();

// bg_slice_sort is an implementation of quicksort.
size_t
bg_slice_sort(struct bg_slice *s, void *ctx, bg_slice_sort_key_fn key_fn,
              bg_slice_sort_comparator comparator)
{
    if (s == NULL || key_fn == NULL || s->len <= 1) {
        return 0;
    }

    if (s->len <= 3) {
        bg_slice_sort_insertion(s, ctx, key_fn, comparator);
        return s->len;
    }

    void *first = bg_slice_get_ptr_begin_offset(s);
    void *last = bg_slice_get_last(s);
    size_t mid = s->len / 2;
    void *mid_item = bg_slice_get(s, mid);
    void *pivot = mid_item;
    swap(pivot, last, s->elem_size);
    size_t left_idx = 0;
    size_t right_idx = s->len - 2;
    while (left_idx <= right_idx) {
    }
}