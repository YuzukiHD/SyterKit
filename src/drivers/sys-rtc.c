/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <log.h>
#include <mmu.h>
#include <cache.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <types.h>

#include <reg-ncat.h>

#include <sys-rtc.h>

/**
 * Write data to the RTC register at the specified index.
 *
 * @param index The index of the RTC register to write data to.
 * @param val The value to write to the RTC register.
 */
void rtc_write_data(int index, uint32_t val) {
	writel(val, SUNXI_RTC_DATA_BASE + index * 4);
}

/**
 * Read data from the RTC register at the specified index.
 *
 * @param index The index of the RTC register to read data from.
 * @return The value read from the RTC register.
 */
uint32_t rtc_read_data(int index) {
	return readl(SUNXI_RTC_DATA_BASE + index * 4);
}

/**
 * Set the FEL (Fastboot External Loader) flag in the RTC register.
 * This flag indicates that the system should enter Fastboot mode.
 */
void rtc_set_fel_flag(void) {
	do {
		rtc_write_data(RTC_FEL_INDEX, EFEX_FLAG);
		data_sync_barrier();
	} while (rtc_read_data(RTC_FEL_INDEX) != EFEX_FLAG);
}

/**
 * Set the start time in milliseconds in the RTC register.
 * This function sets the start time for a specific operation.
 */
void rtc_set_start_time_ms(void) {
	uint32_t init_time_ms = get_init_timestamp();
	do {
		rtc_write_data(RTC_FEL_INDEX, init_time_ms);
		data_sync_barrier();
	} while (rtc_read_data(RTC_FEL_INDEX) != init_time_ms);
}

/**
 * @brief Sets the parameters for Dynamic Random Access Memory (DRAM).
 * 
 * This function is used to set the parameters for DRAM and ensures the success of parameter setting through a loop.
 * 
 * @param dram_para_addr Address for DRAM parameters to be set.
 * 
 * @return None.
 * 
 * @note This function continuously attempts to set DRAM parameters until it succeeds.
 */
void rtc_set_dram_para(uint32_t dram_para_addr) {
	do {
		rtc_write_data(RTC_DRAM_PARA_ADDR, dram_para_addr);
		data_sync_barrier();
	} while (rtc_read_data(RTC_DRAM_PARA_ADDR) != dram_para_addr);
}

/**
 * Probe the FEL (Fastboot External Loader) flag in the RTC register.
 *
 * @return The value of the FEL flag (0 or 1).
 */
uint32_t rtc_probe_fel_flag(void) {
	return rtc_read_data(RTC_FEL_INDEX) == EFEX_FLAG ? 1 : 0;
}

/**
 * Clear the FEL (Fastboot External Loader) flag in the RTC register.
 * This function clears the FEL flag after it has been processed.
 */
void rtc_clear_fel_flag(void) {
	do {
		rtc_write_data(RTC_FEL_INDEX, 0);
		data_sync_barrier();
	} while (rtc_read_data(RTC_FEL_INDEX) != 0);
}

/**
 * Set the bootmode flag in the RTC register.
 *
 * @param flag The bootmode flag value to set.
 * @return 0 if successful, or an error code if failed.
 */
int rtc_set_bootmode_flag(uint8_t flag) {
	do {
		rtc_write_data(RTC_BOOT_INDEX, flag);
		data_sync_barrier();
	} while (rtc_read_data(RTC_BOOT_INDEX) != flag);

	return 0;
}

/**
 * Get the bootmode flag from the RTC register.
 *
 * @return The value of the bootmode flag.
 */
int rtc_get_bootmode_flag(void) {
	uint32_t boot_flag;

	/* operation should be same with kernel write rtc */
	boot_flag = rtc_read_data(RTC_BOOT_INDEX);

	return boot_flag;
}
