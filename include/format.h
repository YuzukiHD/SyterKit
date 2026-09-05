/* SPDX-License-Identifier: GPL-2.0+ */
/** @file format.h
 *  @brief Small callback-based formatter used by firmware logging.
 */

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <stdarg.h>

/**
 * @brief Character sink used by the firmware formatter.
 *
 * The callback is invoked once for every output character.  @p arg is passed
 * unchanged on each invocation and can point to a UART, buffer, or other
 * caller-owned output context.
 *
 * @param[in] arg Opaque caller-provided output context.
 * @param[in] value Character to emit.
 */
typedef void (*format_putc_t)(void *arg, char value);

/**
 * @brief Format a variadic argument list through a character callback.
 *
 * Supported conversions are @c %%, @c %c, @c %s, signed decimal @c %d/@c %i,
 * unsigned decimal @c %u, octal @c %o, hexadecimal @c %x/@c %X, and pointers
 * @c %p.  The implementation supports the flags @c -, @c 0, @c +, space,
 * and @c #, numeric or @c * width and precision, and @c l, @c ll, @c z, @c t,
 * and @c j length modifiers.
 *
 * @param[in] putc Character output callback; it must not be `NULL`.
 * @param[in] arg Opaque context passed to @p putc.
 * @param[in] fmt Null-terminated format string.
 * @param[in] args Variadic arguments matching @p fmt.
 * @return Number of characters delivered to @p putc.
 */
unsigned vformat(format_putc_t putc, void *arg, const char *fmt, va_list args);

/**
 * @brief Format variadic arguments through a character callback.
 *
 * This convenience wrapper initializes and disposes the @c va_list before
 * delegating to ::vformat.  It has the same conversion and flag support.
 *
 * @param[in] putc Character output callback; it must not be `NULL`.
 * @param[in] arg Opaque context passed to @p putc.
 * @param[in] fmt Null-terminated format string.
 * @param[in] ... Values consumed according to @p fmt.
 * @return Number of characters delivered to @p putc.
 */
unsigned format(format_putc_t putc, void *arg, const char *fmt, ...);

#endif
