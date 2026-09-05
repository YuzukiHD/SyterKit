/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PSRAM_H__
#define __DRIVERS_PSRAM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SUNXI_PSRAM_MAX_PARAM_WORDS 128U

typedef struct {
	uint32_t parameters[SUNXI_PSRAM_MAX_PARAM_WORDS];
	size_t parameter_count;
	int dt_node;
	uint32_t size; /**< PSRAM size in MB, as reported by the init library. */
	uintptr_t memory_base; /**< CPU-visible PSRAM base. */
	size_t memory_size; /**< Size of the CPU-visible PSRAM address window. */
} sunxi_psram_t;

/**
 * @brief Get the size of the system PSRAM.
 *
 * This function retrieves the total size of the system PSRAM in megabytes.
 *
 * @param psram PSRAM instance to query.
 * @return The size of the PSRAM in MB.
 */
uint32_t sunxi_get_psram_size(const sunxi_psram_t *psram);

/**
 * @brief Initialize the PSRAM controller.
 *
 * This function initializes the embedded PSRAM/LPSRAM using the parameters
 * supplied by the caller. On platforms that ship a prebuilt PSRAM init
 * library this dispatches into it.
 *
 * @param psram PSRAM instance containing the mutable initialization parameters.
 * @return The detected PSRAM size in MB, or zero on failure.
 */
uint32_t sunxi_psram_init(sunxi_psram_t *psram);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DRIVERS_PSRAM_H__
