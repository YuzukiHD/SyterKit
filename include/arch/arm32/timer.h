/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <types.h>

#include "log.h"

uint64_t get_arch_counter(void);

uint32_t time_ms(void);

uint64_t time_us(void);

void udelay(uint64_t us);

void mdelay(uint32_t ms);

void sdelay(uint32_t loops);

uint32_t get_init_timestamp();

#endif// __SYS_TIMER_H__