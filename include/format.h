/* SPDX-License-Identifier: GPL-2.0+ */
/** @file format.h
 *  @brief Small callback-based formatter used by firmware logging.
 */

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <stdarg.h>

typedef void (*format_putc_t)(void *arg, char value);

unsigned vformat(format_putc_t putc, void *arg, const char *fmt, va_list args);
unsigned format(format_putc_t putc, void *arg, const char *fmt, ...);

#endif
