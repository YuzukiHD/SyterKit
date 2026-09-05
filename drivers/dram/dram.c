/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "dram: " fmt

/**
 * @file
 * @brief System DRAM (Dynamic Random Access Memory) driver for Allwinner (sunxi) platforms
 * @details This file provides weak implementations for DRAM-related functions that
 *          can be overridden by platform-specific implementations. These functions
 *          handle DRAM initialization and size detection for Allwinner SoCs.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <drivers/dram/dram.h>

/**
 * @brief Get the total DRAM size
 * @details This weak function returns the total size of the system DRAM in bytes.
 *          Platform-specific implementations should override this function to return
 *          the actual DRAM size detected for the specific SoC and board configuration.
 * @param dram DRAM instance to query.
 * @return Total DRAM size in bytes. Default implementation returns 0.
 */
uint32_t sunxi_get_dram_size(const sunxi_dram_t *dram)
{
	return dram != NULL ? dram->size : 0U;
}

/**
 * @brief Initialize DRAM controller and memory
 * @details This weak function initializes the DRAM controller and configures memory settings.
 *          Platform-specific implementations should override this function to perform
 *          SoC-specific DRAM initialization including timing configuration, voltage setup,
 *          and memory training.
 * @param dram DRAM instance containing the initialization parameters. The exact
 *             structure depends on the platform implementation.
 * @return 0 on success, non-zero error code on failure. Default implementation returns 0.
 */
uint32_t __attribute__((weak)) sunxi_dram_init(sunxi_dram_t *dram)
{
	return dram != NULL ? dram->size : 0U;
}
