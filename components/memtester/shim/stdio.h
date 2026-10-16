/* SPDX-License-Identifier: GPL-2.0+ */
/* The few stdio/stdlib names tests.c uses, on top of the SyterKit console. */
#ifndef MEMTESTER_SHIM_STDIO_H
#define MEMTESTER_SHIM_STDIO_H

#include <log.h>

typedef int FILE;
#define stdout 0
#define stderr 0
#define fflush(f)	((void)(f))
#define fprintf(f, ...) printf(__VA_ARGS__)

int putchar(int c);
int rand(void);

#endif
