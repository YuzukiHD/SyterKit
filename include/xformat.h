/**
 * @file        xformatc.h
 *
 * @brief       Printf C declaration.
 *
 * @author      Mario Viara
 *
 * @version     1.01
 *
 * @copyright   Copyright Mario Viara 2014  - License Open Source (LGPL)
 * This is a free software and is opened for education, research and commercial
 * developments under license policy of following terms:
 * - This is a free software and there is NO WARRANTY.
 * - No restriction on use. You can use, modify and redistribute it for personal,
 *   non-profit or commercial product UNDER YOUR RESPONSIBILITY.
 * - Redistributions of source code must retain the above copyright notice.
 *
 */
#ifndef __XFORMAT_H__
#define __XFORMAT_H__

#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Define internal parameters as volatile for 8 bit cpu define
 * XCFG_FORMAT_STATIC=static to reduce stack usage.
 */
#define XCFG_FORMAT_STATIC

/**
 * Define XCFG_FORMAT_FLOAT=0 to remove floating point support
 */
#define XCFG_FORMAT_FLOAT 1

/**
 * Define to 1 to support long long type (prefix ll)
 */
#define XCFG_FORMAT_LONGLONG 1

/**
 * Formats and outputs a string according to a format string 'fmt' and a variable argument list 'args'.
 * The output is written character by character using a user-defined output function 'outchar'.
 *
 * @param outchar The output function that writes a single character.
 * @param arg A pointer to optional arguments for the output function.
 * @param fmt The format string specifying the output format.
 * @param args The variable argument list containing the values to be formatted and output.
 * @return The number of characters written.
 */
unsigned xvformat(void (*outchar)(void *arg, char), void *arg, const char *fmt, va_list args);

/**
 * Formats and outputs a string according to a format string 'fmt' and an arbitrary number of variable arguments.
 * The output is written character by character using a user-defined output function 'outchar'.
 *
 * @param outchar The output function that writes a single character.
 * @param arg A pointer to optional arguments for the output function.
 * @param fmt The format string specifying the output format.
 * @param ... The variable arguments containing the values to be formatted and output.
 * @return The number of characters written.
 */
unsigned xformat(void (*outchar)(void *arg, char), void *arg, const char *fmt, ...);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif
