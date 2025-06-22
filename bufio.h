#ifndef BUFIO_H
#define BUFIO_H

#include <stdbool.h>

#include "slice.h"

typedef struct Bufio_Printf {
	Slice *str;
} Bufio_Printf;

Bufio_Printf *bufio_printf_new();
void bufio_printf_free(Bufio_Printf *buffprintf);
void buff_printf(Bufio_Printf *buffprintf, const char *__restrict __fmt, ...);

#endif