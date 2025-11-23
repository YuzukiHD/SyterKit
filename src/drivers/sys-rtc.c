/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sys-rtc.c
 * @brief System RTC (Real-Time Clock) driver for Allwinner (sunxi) platforms
 * @details This file implements RTC functionality for Allwinner SoCs, providing
 *          interface functions to read from and write to RTC registers. The RTC
 *          is used to store various persistent flags and parameters, including
 *          FEL (Fastboot External Loader) mode flag, boot mode information,
 *          DRAM parameters, and initialization timestamp. These values are
 *          retained even when the system is powered off, as long as there is
 *          backup battery power.
 */

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
 * @brief Write data to an RTC register
 * @details Writes a 32-bit value to the RTC register at the specified index.
 * @param index The index of the RTC register to write data to
 * @param val The value to write to the RTC register
 */
void rtc_write_data(int index, uint32_t val) {
	writel(val, SUNXI_RTC_DATA_BASE + index * 4);
}

/**
 * @brief Read data from an RTC register
 * @details Reads a 32-bit value from the RTC register at the specified index.
 * @param index The index of the RTC register to read data from
 * @return The value read from the RTC register
 */
uint32_t rtc_read_data(int index) {
	return readl(SUNXI_RTC_DATA_BASE + index * 4);
}

/**
 * @brief Set the FEL (Fastboot External Loader) flag
 * @details Sets the FEL flag in the RTC register, which indicates that the system
 *          should enter Fastboot mode. The function verifies the write operation
 *          was successful by reading back the value.
 */
void rtc_set_fel_flag(void) {
	do {
		rtc_write_data(RTC_FEL_INDEX, EFEX_FLAG);
		data_sync_barrier();
	} while (rtc_read_data(RTC_FEL_INDEX) != EFEX_FLAG);
}

/**
 * @brief Set the initialization timestamp
 * @details Stores the initialization timestamp (in milliseconds) in the RTC register.
 *          This value can be used for timing operations or to determine the system
 *          uptime. The function verifies the write operation was successful.
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
 * @brief Check if the FEL flag is set
 * @details Reads the FEL flag from the RTC register and returns whether it is set.
 * @return 1 if the FEL flag is set, 0 otherwise
 */
uint32_t rtc_probe_fel_flag(void) {
	return rtc_read_data(RTC_FEL_INDEX) == EFEX_FLAG ? 1 : 0;
}

/**
 * @brief Clear the FEL (Fastboot External Loader) flag
 * @details Clears the FEL flag in the RTC register after it has been processed.
 *          The function verifies the write operation was successful by reading back the value.
 */
void rtc_clear_fel_flag(void) {
	do {
		rtc_write_data(RTC_FEL_INDEX, 0);
		data_sync_barrier();
	} while (rtc_read_data(RTC_FEL_INDEX) != 0);
}

/**
 * @brief Set the boot mode flag
 * @details Sets the boot mode flag in the RTC register, which determines how the
 *          system will boot. The function verifies the write operation was successful.
 * @param flag The boot mode flag value to set
 * @return 0 if successful
 */
int rtc_set_bootmode_flag(uint8_t flag) {
	do {
		rtc_write_data(RTC_BOOT_INDEX, flag);
		data_sync_barrier();
	} while (rtc_read_data(RTC_BOOT_INDEX) != flag);

	return 0;
}

/**
 * @brief Get the boot mode flag
 * @details Reads the boot mode flag from the RTC register. This is used to determine
 *          how the system should boot. The operation is designed to be compatible
 *          with how the kernel reads this value.
 * @return The value of the boot mode flag
 */
int rtc_get_bootmode_flag(void) {
	uint32_t boot_flag;

	/* operation should be same with kernel write rtc */
	boot_flag = rtc_read_data(RTC_BOOT_INDEX);

	return boot_flag;
}
