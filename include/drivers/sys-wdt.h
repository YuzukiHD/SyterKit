/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_WDT_H__
#define __SYS_WDT_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

void sys_reset();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif// __SYS_WDT_H__