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

void sys_reset();

#endif// __SYS_WDT_H__