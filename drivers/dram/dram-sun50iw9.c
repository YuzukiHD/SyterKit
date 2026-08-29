/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file dram-sun50iw9.c
 * @brief DRAM controller driver for the Allwinner sun50iw9 SoC.
 *
 * Copies the packed DRAM initialization blob into a scratch region, records
 * the DRAM parameters and start time in the RTC, executes the blob and reads
 * back the detected DRAM size.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <log.h>

#include <dt2c/driver.h>
#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>

extern uint8_t __ddr_bin_start[];
extern uint8_t __ddr_bin_end[];

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM configuration, copies the packed DRAM init blob into the
 * configured scratch region, records the parameters and start time in the
 * RTC, then jumps into the blob. The detected size is read back from the RTC
 * afterwards.
 *
 * @param[in] dram DRAM configuration block.
 *
 * @return Detected DRAM size in bytes, or 0 on failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	uintptr_t image_start = (uintptr_t)__ddr_bin_start;
	uintptr_t image_end = (uintptr_t)__ddr_bin_end;
	const uint8_t *src;
	uint8_t *dst;
	uint32_t *para_data;
	size_t image_size;

	if (dram == NULL || dram->parameter_count == 0U || dram->rtc.data_base == 0U || dram->init_code_base == 0U || dram->init_code_size == 0U || image_end <= image_start) {
		printk_error("DRAM: please provide DRAM para\n");
		return 0U;
	}
	image_size = (size_t)(image_end - image_start);
	if (image_size > dram->init_code_size || dram->init_code_base + dram->init_code_size < dram->init_code_base) {
		printk_error("DRAM: init code region is too small\n");
		return 0U;
	}
	src = (const uint8_t *)image_start;
	dst = (uint8_t *)dram->init_code_base;

	para_data = dram->parameters;

	/* Set DRAM driver clk and training data to */
	if (para_data[0] != 0x0) {
		rtc_set_dram_para(&dram->rtc, (uint32_t)(uintptr_t)para_data);
	}

	printk_debug("DRAM: load dram init from 0x%08lx -> 0x%08lx size: %08lx\n", (unsigned long)image_start, (unsigned long)dram->init_code_base, (unsigned long)image_size);
	memcpy(dst, src, image_size);

	/* Set RTC data to current time_ms(), Save in RTC_FEL_INDEX */
	rtc_set_start_time_ms(&dram->rtc);

	printk_debug("DRAM: Now jump to 0x%08lx run DRAMINIT\n", (unsigned long)dram->init_code_base);

	__asm__ __volatile__("isb sy" : : : "memory");
	__asm__ __volatile__("dsb sy" : : : "memory");
	__asm__ __volatile__("dmb sy" : : : "memory");
	((void (*)(void))((void *)dram->init_code_base))();

	dram->size = rtc_read_data(&dram->rtc, RTC_FEL_INDEX);

	/* And Restore RTC Flag */
	rtc_clear_fel_flag(&dram->rtc);

	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
