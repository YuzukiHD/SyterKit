/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "log.h"

uint64_t get_arch_counter(void);

uint32_t time_ms(void);

uint64_t time_us(void);

void udelay(uint64_t us);

void mdelay(uint32_t ms);

void sdelay(uint32_t loops);

#endif // __SYS_TIMER_H__