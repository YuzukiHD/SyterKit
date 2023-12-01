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
 * Define to 0 to support long long type (prefix ll)
 */
#ifndef XCFG_FORMAT_LONGLONG
#define XCFG_FORMAT_LONGLONG 0
#endif

unsigned xvformat(void (*outchar)(void *arg, char), void *arg, const char *fmt, va_list args);
unsigned xformat(void (*outchar)(void *arg, char), void *arg, const char *fmt, ...);

#endif
