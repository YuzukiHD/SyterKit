/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/clk/sun8iw22/reg.h>
#include <dt2c/driver.h>

#define CPU_UPDATE_OFFSET (26)
#define CPU_LOCK_OFFSET (28)
#define CPU_LOCK_ENABLE_OFFSET (29)

void sunxi_clk_init(sunxi_ccu_t *ccu) {
	uint32_t reg_val;
	uint32_t judge_cnt = 0;

	/* Configure PLL CPU: n=0x2a, m0=m1=1, p=1, CPU PLL 1008MHz */
	/* 24M*n/p/(m0 * m1) */
	clrsetbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG),
			(0xffU << 8) | (0x7U << 16) | (0x3U << 20) | (0xfU << 0),
			(0x2a << 8) | (0x0 << 0));
	udelay(10);

	/* Update PLL */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG),
		     (0x1U << CPU_UPDATE_OFFSET));
	/* Wait for hardware to clear update bit */
	while (readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG)) &
	       (0x1U << CPU_UPDATE_OFFSET))
		;

	/* Lock enable */
	clrbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG),
		     (0x1U << CPU_LOCK_ENABLE_OFFSET));
	udelay(10);
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG),
		     (0x1U << CPU_LOCK_ENABLE_OFFSET));

	/* Wait for lock */
	judge_cnt = 0;
	do {
		udelay(3);
		reg_val = readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG));
		judge_cnt = (reg_val & (0x1U << CPU_LOCK_OFFSET)) ? judge_cnt + 1 : 0;
		/* Must judge 3 times continuously for stable lock */
	} while (judge_cnt < 3);
	udelay(20);

	/* Set CPU divider factor P */
	clrbits_le32(ccu->base[1] + 0x20U, (0x3 << 16));

	/* Set CPU-AXI divider factor M */
	clrsetbits_le32(ccu->base[1] + 0x20U,
			(0x3 << 0), (0x1 << 0));
	udelay(10);

	/* Set CPU clock source */
	clrsetbits_le32(ccu->base[1] + 0x18U,
			(0x7 << 24), (0x3 << 24));
	udelay(10);
}

void sunxi_clk_dump(sunxi_ccu_t *ccu) {
	(void) ccu;
}

DT2C_DRIVER_COMPAT("allwinner,sun8iw22-ccu");
