/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN50IW9_SYS_SID_H__
#define __SUN50IW9_SYS_SID_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#include <reg-ncat.h>

uint32_t efuse_read(uint32_t offset);

void efuse_write(uint32_t offset, uint32_t value);

void dump_efuse(void);

#endif// __SUN50IW9_SYS_SID_H__