/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SOC_SOC_H__
#define __DRIVERS_SOC_SOC_H__

#include <stddef.h>
#include <stdint.h>

#define SUNXI_SOC_COMPATIBLE "allwinner,soc"

/**
 * @struct sunxi_soc
 * @brief SoC identification register window.
 *
 * Populated from the allwinner,soc device-tree node.
 */
typedef struct sunxi_soc {
	int dt_node;    /**< Device-tree node offset. */
	uintptr_t base; /**< Register window base address. */
	size_t size;    /**< Register window size. */
} sunxi_soc_t;

/**
 * @brief Read the SoC platform identifier.
 *
 * Latches the die/batch information into the upper 16 bits of the version
 * register and returns it.  The register window base comes from the device
 * tree.  The read returns zero when no usable allwinner,soc node is present.
 *
 * @return The die/batch platform identifier, or 0 on error.
 */
uint32_t sunxi_soc_platform_id(void);

#endif /* __DRIVERS_SOC_SOC_H__ */
