/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_RTC_H__
#define __SYS_RTC_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

void rtc_write_data(int index, uint32_t val);

uint32_t rtc_read_data(int index);

void rtc_set_fel_flag(void);

void rtc_set_start_time_ms(void);

uint32_t rtc_probe_fel_flag(void);

void rtc_clear_fel_flag(void);

int rtc_set_bootmode_flag(uint8_t flag);

int rtc_get_bootmode_flag(void);

#endif// __SYS_RTC_H__