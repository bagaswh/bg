
#include "bufio.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "slice.h"

Bufio_Printf *bufio_printf_new() {
	return bufio_print_new_with_bufsize(65536);  // defaults to 64k
}

Bufio_Printf *bufio_print_new_with_bufsize(size_t bufsize) {
	Bufio_Printf *pbuf = malloc(sizeof(Bufio_Printf));
	pbuf->str = slice_char_new(bufsize, bufsize);
	return pbuf;
}

Bufio_Printf *bufio_printf_realloc(Bufio_Printf *pbuf, size_t len) {
	if (pbuf == NULL) {
		return NULL;
	}
	if (pbuf->str == NULL) {
		return NULL;
	}
	pbuf->str = slice_grow_to_cap(pbuf->str, len);
	return pbuf;
}

void bufio_printf_free(Bufio_Printf *pbuf) {
	if (pbuf == NULL) {
		return;
	}
	if (pbuf->str != NULL) {
		free(pbuf->str);
	}
	free(pbuf);
	return;
}

void bufio_printf_append(Bufio_Printf *pbuf, const char *__restrict __fmt, ...) {
	va_list args;
	va_start(args, __fmt);

	va_list args_copy;
	va_copy(args_copy, args);
	int needed = vsnprintf(NULL, 0, __fmt, args_copy);
	va_end(args_copy);
	if (needed < 0) {
		va_end(args);
		return -1;
	}

	size_t required_capacity = slice_get_len(pbuf->str) + needed + 1;
	if (required_capacity > slice_get_cap(pbuf->str)) {
		pbuf->str = slice_grow_to_cap(pbuf->str, required_capacity);
		if (!pbuf->str) {
			va_end(args);
			return -1;
		}
	}

	char *write_position = (char *)slice_get_ptr_offset(pbuf->str);
	int result = vsnprintf(write_position, slice_get_usable_cap(pbuf->str), __fmt, args);
	va_end(args);

	if (result > 0) {
		slice_incr_len(pbuf->str, result);
	}

	return result;
}

void buff_printf(Bufio_Printf *pbuf, const char *__restrict __fmt, ...) {
	va_list args;
	va_start(args, __fmt);
}