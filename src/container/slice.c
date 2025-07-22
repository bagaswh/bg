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

#define assert_slice_bound_check(sl, i)                      \
  do {                                                       \
    if (unlikely(abort_on_oob && (i >= sl->len || i < 0))) { \
      bg_assert("BGSlice",                                   \
                "tried perform out-of-bound access at "      \
                "index %d "                                  \
                "of slice length %d\n",                      \
                i, sl->len);                                 \
    }                                                        \
  } while (0)

#define assert_slice(condition, fmt, ...)              \
  do {                                                 \
    bg_assert("BGSlice", condition, fmt, __VA_ARGS__); \
  } while (0)

typedef struct BGSlice_s {
  size_t cap;
  size_t len;
  size_t elem_size;
  void *data;
  struct Allocator *allocator;
} BGSlice_s;

struct BGSliceOption *BGSlice_with_allocator(struct BGSliceOption *option,
                                             struct Allocator *allocator) {
  assert_slice(allocator != NULL, "allocator cannot be NULL");
  option->allocator = allocator;
  return option;
}

struct Allocator *get_allocator(struct BGSliceOption *option) {
  struct Allocator *allocator = malloc_allocator;
  if (option != NULL && option->allocator != NULL)
    allocator = option->allocator;
  return allocator;
}

void *BGSlice_allocate_backing_buffer(size_t cap, size_t len, size_t elem_size,
                                      struct BGSliceOption *option) {
  struct Allocator *allocator = get_allocator(option);

  void *data = allocator->calloc(cap, elem_size);
  if (data == NULL) return NULL;

  return data;
}

struct BGSlice *allocate_slice(struct BGSliceOption *option) {
  struct Allocator *allocator = get_allocator(option);
  return allocator->malloc(sizeof(BGSlice_s));
}

struct BGSlice *BGSlice_new(size_t cap, size_t len, size_t elem_size,
                            struct BGSliceOption *option) {
  if (cap == 0 || elem_size == 0) return NULL;

  struct Allocator *allocator = get_allocator(option);
  void *data = BGSlice_allocate_backing_buffer(cap, len, elem_size, option);
  if (data == NULL) return NULL;

  struct BGSlice_s *s = allocate_slice(option);
  if (s == NULL) return NULL;

  s->allocator = allocator;
  s->cap = cap;
  s->len = len;
  s->elem_size = elem_size;
  s->data = data;

  return s;
}

struct BGSlice *BGSlice_copy_from_ptr(void *ptr, size_t cap, size_t len,
                                      size_t elem_size,
                                      struct BGSliceOption *option) {
  if (cap == 0 || elem_size == 0) return NULL;

  struct Allocator *allocator = get_allocator(option);
  void *data = BGSlice_allocate_backing_buffer(cap, len, elem_size, option);
  if (data == NULL) return NULL;

  struct BGSlice_s *s = allocate_slice(option);
  if (s == NULL) return NULL;

  s->allocator = allocator;
  s->cap = cap;
  s->len = len;
  s->elem_size = elem_size;
  s->data = data;
  memcpy(s->data, ptr, BGSlice_get_len_in_bytes(s));

  return s;
}

struct BGSlice *BGSlice_new_from_ptr(void *ptr, size_t cap, size_t len,
                                     size_t elem_size,
                                     struct BGSliceOption *option) {
  if (cap == 0 || elem_size == 0) return NULL;

  struct Allocator *allocator = get_allocator(option);

  struct BGSlice_s *s = allocate_slice(option);
  if (s == NULL) return NULL;

  s->allocator = allocator;
  s->cap = cap;
  s->len = len;
  s->elem_size = elem_size;
  s->data = ptr;

  return s;
}

struct BGSlice *BGSlice_char_new(size_t cap, size_t len,
                                 struct BGSliceOption *option) {
  return BGSlice_new(cap, len, sizeof(char), option);
}

struct BGSlice *BGSlice_char_new_from_str(const char *str,
                                          struct BGSliceOption *option) {
  return BGSlice_copy_from_ptr(str, strlen(str) + 1, strlen(str) + 1,
                               sizeof(char), option);
}

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
 * implementation dependent).
 *
 */
struct BGSlice *BGSlice_new_from_slice(struct BGSlice *__src, size_t start,
                                       ssize_t len,
                                       struct BGSliceOption *option) {
  struct BGSlice_s *src = __src;
  if (start >= src->cap || len > src->cap) return NULL;

  struct BGSlice_s *s = allocate_slice(option);
  if (s == NULL) return NULL;

  memcpy(s, src, sizeof(struct BGSlice_s));

  if (len == -1)
    s->len = src->len - start;
  else
    s->len = len - start;

  s->data = src->data + BGSlice_get_size_in_bytes(src, start);

  return s;
}

/*
 * Create a new slice from a slice, using new backing buffer.
 */
struct BGSlice *BGSlice_copy_from_slice(struct BGSlice *__src, size_t start,
                                        ssize_t len, ssize_t cap,
                                        struct BGSliceOption *option) {
  struct BGSlice_s *src = __src;
  if (start >= src->cap || len > src->cap) return NULL;

  struct BGSlice_s *s = allocate_slice(option);
  if (s == NULL) return NULL;
  memcpy(s, src, sizeof(struct BGSlice_s));

  if (len == -1)
    s->len = src->len - start;
  else
    s->len = len - start;

  if (cap == -1)
    s->cap = src->cap - start;
  else
    s->cap = cap - start;

  void *data =
      BGSlice_allocate_backing_buffer(cap, len, src->elem_size, option);
  if (data == NULL) return NULL;

  memcpy(s->data, src->data + BGSlice_get_size_in_bytes(src, start),
         BGSlice_get_size_in_bytes(src, s->len));

  return s;
}

struct BGSlice *BGSlice_reset(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) return NULL;
  s->len = 0;
  return s;
}

ssize_t BGSlice_get_len(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) {
    return -1;
  }
  return s->len;
}

void BGSlice_set_len(struct BGSlice *__s, size_t len) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) {
    return;
  }
  if (len > s->cap || len < 0) {
    return;
  }
  s->len = len;
  return;
}

void BGSlice_incr_len(struct BGSlice *__s, size_t incr) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) {
    return;
  }
  if (unlikely((s->len + incr) > s->cap || (s->len + incr) < 0)) {
    return;
  }
  s->len += incr;
  return;
}

ssize_t BGSlice_get_cap(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) {
    return -1;
  }
  return s->cap;
}

ssize_t BGSlice_get_usable_cap(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) {
    return -1;
  }
  return s->data == NULL ? -1 : s->cap - s->len;
}

void BGSlice_free(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s != NULL && s->data != NULL)) free(s->data);
  if (unlikely(s != NULL)) free(s);
}

size_t BGSlice_new_cap(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  size_t new_cap = 0;
  if (s->cap < 256)
    new_cap = s->cap * 2;
  else
    new_cap = s->cap * 1.25;
  return new_cap;
}

struct BGSlice *BGSlice_grow(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) return NULL;
  s->cap = BGSlice_new_cap(s);
  s->data = s->allocator->realloc(s->data, BGSlice_get_cap_in_bytes(s));
  if (s->data == NULL) return NULL;
  return s;
}

void *BGSlice_get_ptr_offset(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) return NULL;
  return s->data + BGSlice_get_len_in_bytes(s);
}

void *BGSlice_get_ptr_begin_offset(struct BGSlice *__s) {
  struct BGSlice_s *s = __s;
  if (unlikely(s == NULL)) return NULL;
  return s->data;
}

struct BGSlice *BGSlice_grow_to_cap(struct BGSlice *__s, size_t cap) {
  struct BGSlice_s *s = __s;
  if (cap < s->cap) return s;
  s->data = s->allocator->realloc(s->data, BGSlice_get_size_in_bytes(s, cap));
  s->cap = cap;
  if (s->data == NULL) return NULL;
  return s;
}

struct BGSlice *BGSlice_append(struct BGSlice *__s, void *const item) {
  struct BGSlice_s *s = __s;
  if (unlikely(s->len >= s->cap)) {
    s = BGSlice_grow(s);
    if (s == NULL) return NULL;
  }
  char *ptr = BGSlice_get_ptr_offset(s);
  memcpy(ptr, item, s->elem_size);
  s->len++;
  return s;
}

struct BGSlice *BGSlice_append_n(struct BGSlice *__s, void *const item,
                                 size_t n) {
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

bool BGSlice_is_full(struct BGSlice *__s) {
  if (s != NULL && s->len >= s->cap) {
    return true;
  }
  return false;
}

ssize_t BGSlice_copy(struct BGSlice *dst, struct BGSlice *src) {
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
  memcpy(dst->data, src->data, BGSlice_get_size_in_bytes(dst, len_to_copy));
  return len_to_copy;
}

void *BGSlice_get(struct BGSlice *s, size_t index) {
  if (unlikely(s == NULL || index >= s->len)) {
    assert_slice_bound_check(sl, index);
    return NULL;
  }
  return &(((char *)s->data)[BGSlice_get_size_in_bytes(s, index)]);
}

void *BGSlice_get_last(struct BGSlice *s) {
  if (unlikely(s == NULL || s->len == 0)) {
    assert_slice_bound_check(sl, index);
    return NULL;
  }
  return &(((char *)s->data)[BGSlice_get_size_in_bytes(s, s->len - 1)]);
}

void BGSlice_range(struct BGSlice *s, void *ctx,
                   BGSlice_range_callback callback) {
  if (unlikely(s == NULL || callback == NULL)) {
    return;
  }
  for (size_t i = 0; i < s->len; i++) {
    char *ptr = s->data + BGSlice_get_size_in_bytes(s, i);
    if (!callback(ptr, i, ctx)) {
      break;
    }
  }
}

typedef u8 *comparable;
typedef comparable (*BGSlice_sort_get_key_fn)(void *item, size_t idx);

void swap(void *a, void *b, size_t size) {
  char tmp[size];
  memcpy(tmp, a, size);
  memcpy(a, b, size);
  memcpy(b, tmp, size);
}

typedef ssize_t (*BGSlice_sort_comparator)(comparable a, comparable b,
                                           void *ctx);
typedef comparable (*BGSlice_sort_key_fn)(void *item, size_t idx);

comparable BGSlice_get_key_fn_default(void *item, size_t idx) {
  return (comparable)item;
}

size_t BGSlice_sort_insertion(struct BGSlice *s, void *ctx,
                              BGSlice_sort_key_fn key_fn,
                              BGSlice_sort_comparator comparator) {
  if (s == NULL || s->len <= 1) {
    return 0;
  }

  key_fn = key_fn == NULL ? BGSlice_get_key_fn_default : key_fn;

  for (size_t i = 0; i < s->len; i++) {
    ssize_t j = i;

    void *item_a = BGSlice_get(s, j);
    comparable key_a = get_key_fn(item_a, j);
    void *item_b = BGSlice_get(s, j - 1);
    comparable key_b = get_key_fn(item_b, j - 1);
    ssize_t compare_ret = comparator(key_a, key_b, ctx);

    while ((j - 1) >= 0 && compare_ret < 0) {
      swap(item_a, item_b, s->elem_size);
    }
  }
}

void *BGSlice_sort_mo3_median();

void *BGSlice_sort_mo3_partition();

// BGSlice_sort is an implementation of quicksort.
size_t BGSlice_sort(struct BGSlice *s, void *ctx, BGSlice_sort_key_fn key_fn,
                    BGSlice_sort_comparator comparator) {
  if (s == NULL || key_fn == NULL || s->len <= 1) {
    return 0;
  }

  if (s->len <= 3) {
    BGSlice_sort_insertion(s, ctx, key_fn, comparator);
    return s->len;
  }

  void *first = BGSlice_get_ptr_begin_offset(s);
  void *last = BGSlice_get_last(s);
  size_t mid = s->len / 2;
  void *mid_item = BGSlice_get(s, mid);
  void *pivot = mid_item;
  swap(pivot, last, s->elem_size);
  size_t left_idx = 0;
  size_t right_idx = s->len - 2;
  while (left_idx <= right_idx) {
  }
}