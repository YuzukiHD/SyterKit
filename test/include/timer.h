/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTER_TEST_TIMER_H__
#define __SYTER_TEST_TIMER_H__

#include <stdint.h>

uint64_t get_arch_counter(void);
uint32_t time_ms(void);
uint32_t time_us(void);
uint32_t get_init_timestamp(void);
void udelay(uint32_t us);
void mdelay(uint32_t ms);

#endif
