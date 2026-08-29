/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "psram: " fmt

/**
 * @file
 * @brief System PSRAM (Pseudo Static RAM) driver for Allwinner (sunxi) platforms
 * @details This file provides weak implementations for PSRAM-related functions
 *          that can be overridden by platform-specific implementations. These
 *          functions handle PSRAM initialization and size detection for SoCs
 *          that use embedded PSRAM/LPSRAM instead of external DRAM.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <drivers/psram/psram.h>

/**
 * @brief Get the total PSRAM size
 * @details This weak function returns the total size of the system PSRAM in
 *          megabytes. Platform-specific implementations should override this
 *          function to return the actual PSRAM size detected for the specific
 *          SoC and board configuration.
 * @param psram PSRAM instance to query.
 * @return Total PSRAM size in MB. Default implementation returns 0.
 */
uint32_t __attribute__((weak)) sunxi_get_psram_size(const sunxi_psram_t *psram)
{
	return psram != NULL ? psram->size : 0U;
}

/**
 * @brief Initialize PSRAM controller and memory
 * @details This weak function initializes the PSRAM controller and configures
 *          memory settings. Platform-specific implementations should override
 *          this function to perform SoC-specific PSRAM initialization.
 * @param psram PSRAM instance containing the initialization parameters.
 * @return 0 on success, non-zero error code on failure. Default returns 0.
 */
uint32_t __attribute__((weak)) sunxi_psram_init(sunxi_psram_t *psram)
{
	return psram != NULL ? psram->size : 0U;
}
