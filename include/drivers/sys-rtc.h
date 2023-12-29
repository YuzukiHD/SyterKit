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

#define EFEX_FLAG (0x5AA5A55A)
#define RTC_FEL_INDEX 2
#define RTC_BOOT_INDEX 6

/* Write data to the RTC register at the specified index */
void rtc_write_data(int index, uint32_t val);

/* Read data from the RTC register at the specified index */
uint32_t rtc_read_data(int index);

/* Set the FEL flag in the RTC register */
void rtc_set_fel_flag(void);

/* Set the start time in milliseconds in the RTC register */
void rtc_set_start_time_ms(void);

/* Probe the FEL flag in the RTC register */
uint32_t rtc_probe_fel_flag(void);

/* Clear the FEL flag in the RTC register */
void rtc_clear_fel_flag(void);

/* Set the bootmode flag in the RTC register */
int rtc_set_bootmode_flag(uint8_t flag);

/* Get the bootmode flag from the RTC register */
int rtc_get_bootmode_flag(void);

#endif// __SYS_RTC_H__