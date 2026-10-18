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

/**
 * @brief Change the frequency of the CPU PLL after sunxi_clk_init().
 *
 * The core voltage is not touched; raise it first when the new frequency needs it.
 *
 * @param mhz New frequency in MHz (a multiple of 24 on the SoCs that implement this).
 * @return 0 on success, negative when the frequency is not possible or the PLL did not lock.
 */
int sunxi_clk_cpu_set_freq(uint32_t mhz);

/**
 * @brief Run the NPU from its own PLL at @p mhz instead of the 600 MHz of the peripheral PLL.
 *
 * Programs the NPU PLL, waits for the lock and makes it the parent of the NPU module clock, with no divider. The NPU
 * must be idle, and its module clock open (sunxi_npu_init() does that, and chooses the parent again, so call this
 * after it). The supply is not touched; the NPU shares the core rail (AXP333 dcdc1) with the CPU.
 *
 * @param mhz New frequency in MHz, a multiple of 24 up to 6144.
 * @return 0 on success, -1 for a frequency the PLL cannot make, -2 when the PLL did not lock (the clock is left alone).
 */
int sunxi_clk_npu_set_freq(uint32_t mhz);

/** @brief Reset the system clock tree to its boot configuration. */
void sunxi_clk_reset(void);

/** @brief Dump the current system clock configuration. */
void sunxi_clk_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_CLK_H__ */
