/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_RTC_H__
#define __SYS_RTC_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SUNXI_RTC_COMPATIBLE "allwinner,sunxi-rtc"

#define EFEX_FLAG (0x5AA5A55A)
#define RTC_FEL_INDEX 2
#define RTC_DRAM_PARA_ADDR 3
#define RTC_BOOT_INDEX 6

typedef struct sunxi_rtc {
	int dt_node;
	uintptr_t data_base;
	uint32_t data_size;
} sunxi_rtc_t;

/**
 * @brief Write data to the RTC register at the specified index.
 *
 * @param index The index of the RTC register to write data to.
 * @param val The value to write to the RTC register.
 */
void rtc_write_data(const sunxi_rtc_t *rtc, int index, uint32_t val);

/**
 * @brief Read data from the RTC register at the specified index.
 *
 * @param index The index of the RTC register to read data from.
 * @return The value read from the RTC register.
 */
uint32_t rtc_read_data(const sunxi_rtc_t *rtc, int index);

/**
 * @brief Set the FEL (Fastboot External Loader) flag in the RTC register.
 * This flag indicates that the system should enter Fastboot mode.
 */
void rtc_set_fel_flag(const sunxi_rtc_t *rtc);

/**
 * @brief Set the start time in milliseconds in the RTC register.
 * This function sets the start time for a specific operation.
 */
void rtc_set_start_time_ms(const sunxi_rtc_t *rtc);

/**
 * @brief Sets the parameters for Dynamic Random Access Memory (DRAM).
 * 
 * This function is used to set the parameters for DRAM and ensures the success of parameter setting through a loop.
 * 
 * @param dram_para_addr Address for DRAM parameters to be set.
 * 
 * @note This function continuously attempts to set DRAM parameters until it succeeds.
 */
void rtc_set_dram_para(const sunxi_rtc_t *rtc, uint32_t dram_para_addr);

/**
 * @brief Probe the FEL (Fastboot External Loader) flag in the RTC register.
 *
 * @return The value of the FEL flag (0 or 1).
 */
uint32_t rtc_probe_fel_flag(const sunxi_rtc_t *rtc);

/**
 * @brief Clear the FEL (Fastboot External Loader) flag in the RTC register.
 * This function clears the FEL flag after it has been processed.
 */
void rtc_clear_fel_flag(const sunxi_rtc_t *rtc);

/**
 * @brief Set the bootmode flag in the RTC register.
 *
 * @param flag The bootmode flag value to set.
 * @return 0 if successful, or an error code if failed.
 */
int rtc_set_bootmode_flag(const sunxi_rtc_t *rtc, uint8_t flag);

/**
 * @brief Get the bootmode flag from the RTC register.
 *
 * @return The value of the bootmode flag.
 */
int rtc_get_bootmode_flag(const sunxi_rtc_t *rtc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SYS_RTC_H__ */
