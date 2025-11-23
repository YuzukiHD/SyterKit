/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sys-dram.c
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

/**
 * @brief Get the total DRAM size
 * @details This weak function returns the total size of the system DRAM in bytes.
 *          Platform-specific implementations should override this function to return
 *          the actual DRAM size detected for the specific SoC and board configuration.
 * @return Total DRAM size in bytes. Default implementation returns 0.
 */
uint64_t __attribute__((weak)) sunxi_get_dram_size() {
	return 0;
}

/**
 * @brief Initialize DRAM controller and memory
 * @details This weak function initializes the DRAM controller and configures memory settings.
 *          Platform-specific implementations should override this function to perform
 *          SoC-specific DRAM initialization including timing configuration, voltage setup,
 *          and memory training.
 * @param para Pointer to initialization parameters. The exact structure depends on the
 *             platform implementation.
 * @return 0 on success, non-zero error code on failure. Default implementation returns 0.
 */
uint32_t __attribute__((weak)) sunxi_dram_init(void *para) {
	return 0;
}