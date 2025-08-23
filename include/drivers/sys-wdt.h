/* SPDX-License-Identifier: GPL-2.0+ */

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
#endif// __cplusplus

/**
 * Perform a system reset.
 *
 * This function resets the system, causing it to restart.
 * Any unsaved data or state will be lost.
 */
void sys_reset();

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_WDT_H__