/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_CLK_H__
#define __DRIVERS_CLK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uintptr_t gate_reg_base;
	uint32_t gate_reg_offset;
	uintptr_t rst_reg_base;
	uint32_t rst_reg_offset;
	uint32_t parent_clk;
} sunxi_clk_t;

/** @brief Perform SoC-specific clock work required before normal init. */
void sunxi_clk_preinit(void);

/** @brief Initialize the system clock tree. */
void sunxi_clk_init(void);

/** @brief Reset the system clock tree to its boot configuration. */
void sunxi_clk_reset(void);

/** @brief Dump the current system clock configuration. */
void sunxi_clk_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_CLK_H__ */
