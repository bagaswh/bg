
#include "slice.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (a < b ? a : b)
#define SLICE_GET_SIZE_IN_BYTES(sl, size) (sl == NULL ? -1 : size * sl->elem_size)
#define SLICE_GET_CAP_IN_BYTES(sl) SLICE_GET_SIZE_IN_BYTES(sl, sl->cap)
#define SLICE_GET_LEN_IN_BYTES(sl) SLICE_GET_SIZE_IN_BYTES(sl, sl->len)

typedef struct Slice_s {
	size_t cap;
	size_t len;
	size_t elem_size;
	void *data;
} Slice_s;

Slice_s *slice_char_new(size_t cap, size_t len) {
	return slice_new(cap, len, sizeof(char));
}

Slice_s *slice_char_new_from_str(const char *str) {
	Slice_s *s = slice_char_new(strlen(str), strlen(str));
	if (s == NULL) {
		return NULL;
	}
	memcpy(s->data, str, SLICE_GET_SIZE_IN_BYTES(s, s->len));
	return s;
}

/*
 * Create a new slice from a slice, using the same backing buffer.
 * This can be used to change the offset and length of the slice.
 *
 * Another use case is to perform a slice_copy when you only have ptr. You can make a slice from the ptr.
 */
Slice_s *slice_new_from_slice(Slice_s *src, size_t start, size_t len) {
	if (start >= src->cap || len > src->cap) {
		return NULL;
	}

	Slice_s *s = malloc(sizeof(Slice_s));
	if (s == NULL) {
		return NULL;
	}
	memcpy(s, src, sizeof(Slice_s));
	s->len = len;
	s->data = s->data + SLICE_GET_SIZE_IN_BYTES(s, start);
	return s;
}

Slice_s *slice_new(size_t cap, size_t len, size_t elem_size) {
	if (cap == 0 || elem_size == 0) {
		return NULL;
	}

	void *data = calloc(cap, elem_size);
	if (data == NULL) {
		return NULL;
	}

	Slice_s *s = malloc(sizeof(Slice_s));
	if (s == NULL) {
		return NULL;
	}
	s->cap = cap;
	s->len = len;
	s->elem_size = elem_size;
	s->data = data;

	return s;
}

Slice_s *slice_reset(Slice_s *s) {
	if (s == NULL) {
		return NULL;
	}
	s->len = 0;
	return s;
}

ssize_t slice_get_len(Slice_s *s) {
	if (s == NULL) {
		return -1;
	}
	return s->len;
}

void slice_set_len(Slice_s *s, size_t len) {
	if (s == NULL) {
		return;
	}
	if (len > s->cap || len < 0) {
		return;
	}
	s->len = len;
	return;
}

void slice_incr_len(Slice_s *s, size_t incr) {
	if (s == NULL) {
		return;
	}
	if ((s->len + incr) > s->cap || (s->len + incr) < 0) {
		return;
	}
	s->len += incr;
	return;
}

ssize_t slice_get_total_cap(Slice_s *s) {
	if (s == NULL) {
		return -1;
	}
	return s->cap;
}

ssize_t slice_get_usable_cap(Slice_s *s) {
	if (s == NULL) {
		return -1;
	}
	return s->data == NULL ? -1 : s->cap - s->len;
}

void slice_free(Slice_s *s) {
	if (s != NULL && s->data != NULL) {
		free(s->data);
	}

	if (s != NULL) {
		free(s);
	}
}

Slice_s *slice_grow(Slice_s *s) {
	size_t new_cap = 0;
	if (s->cap < 256) {
		s->cap *= 2;
	} else {
		s->cap *= 1.25;
	}
	void *prev_data = s->data;
	s->data = realloc(s->data, SLICE_GET_CAP_IN_BYTES(s));
	if (s->data == NULL) {
		return NULL;
	}
	if (prev_data != s->data) {
		memcpy(s->data, prev_data, SLICE_GET_LEN_IN_BYTES(s));
	}
	return s;
}

void *slice_get_ptr_offset(Slice_s *s) {
	if (s == NULL) {
		return NULL;
	}
	return s->data + SLICE_GET_LEN_IN_BYTES(s);
}

Slice_s *slice_grow_to_cap(Slice_s *s, size_t cap) {
	if (cap < s->cap) {
		return s;
	}
	s->data = realloc(s->data, SLICE_GET_SIZE_IN_BYTES(s, cap));
	s->cap = cap;
	if (s->data == NULL) {
		return NULL;
	}
	return s;
}

Slice_s *slice_append(Slice_s *s, void *const item) {
	if (s->len >= s->cap) {
		s = slice_grow(s);
		if (s == NULL) {
			return NULL;
		}
	}
	char *ptr = s->data + SLICE_GET_SIZE_IN_BYTES(s, s->len);
	memcpy(ptr, item, s->elem_size);
	s->len++;
	return s;
}

bool slice_is_full(Slice_s *s) {
	if (s != NULL && s->len >= s->cap) {
		return true;
	}

	return false;
}

ssize_t slice_copy(Slice_s *dst, Slice_s *src) {
	if (dst == NULL || src == NULL) {
		return -1;
	}
	if (dst->elem_size != src->elem_size) {
		return -1;
	}
	if (dst->data == NULL || src->data == NULL) {
		return -1;
	}
	size_t len_to_copy = MIN(dst->len, src->len);
	memcpy(dst->data, src->data, SLICE_GET_SIZE_IN_BYTES(dst, len_to_copy));
	return len_to_copy;
}

void *slice_get(Slice_s *s, size_t index) {
	if (s == NULL || index > s->len) return NULL;
	return &(((char *)s->data)[SLICE_GET_SIZE_IN_BYTES(s, index)]);
}

void slice_range(Slice *s, bool (*callback)(void *, size_t idx, size_t len, size_t cap)) {
	if (s == NULL || callback == NULL) {
		return;
	}
	size_t idx = 0;
	for (size_t i = 0; i < s->len; i += s->elem_size) {
		if (!callback(&(((char *)s->data)[i]), idx, i, s->len)) {
			break;
		}
		idx++;
	}
}