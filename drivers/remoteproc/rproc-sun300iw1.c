/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt2c/driver.h>
#include <interrupt.h>
#include <io.h>
#include <timer.h>

#include <drivers/clk/sun300iw1/reg.h>

#define A27L_WFI_MODE_REG 0x4U
#define A27L_START_ADDR_REG 0x204U
#define A27L_WAKEUP_CTRL_REG 0x64U
#define A27L_WAKEUP_EN (1U << 8)

enum sun300iw1_a27l2_register {
	SUN300IW1_A27L2_PMU_AON,
	SUN300IW1_A27L2_CCU_AON,
	SUN300IW1_A27L2_CCU_APP,
	SUN300IW1_A27L2_CFG,
};

static int sun300iw1_a27l2_start(sunxi_remoteproc_t *remoteproc)
{
	uint32_t cache_flags = 0x103fU;
	uintptr_t pmu_aon = remoteproc->registers[SUN300IW1_A27L2_PMU_AON].base;
	uintptr_t ccu_aon = remoteproc->registers[SUN300IW1_A27L2_CCU_AON].base;
	uintptr_t ccu_app = remoteproc->registers[SUN300IW1_A27L2_CCU_APP].base;
	uintptr_t cfg = remoteproc->registers[SUN300IW1_A27L2_CFG].base;

	if (remoteproc->entry == 0U)
		return DRIVER_ERROR_INVALID;

	interrupt_disable();
	setbits_le32(pmu_aon + A27L_WAKEUP_CTRL_REG, A27L_WAKEUP_EN);
	writel(A27L2_CLK_REG_A27L2_CLK_EN_CLOCK_IS_ON << A27L2_CLK_REG_A27L2_CLK_EN_OFFSET, ccu_aon + A27L2_CLK_REG);
	writel(CCU_A27L2_MTCLK_EN, ccu_app + CCU_A27L2_MTCLK_REG);
	clrsetbits_le32(ccu_app + CCU_APP_CLK_REG, CCU_APP_CLK_REG_A27L2_BUSCLKDIV_CLEAR_MASK,
			CCU_APP_CLK_REG_A27L2_BUSCLKDIV_DIV2 << CCU_APP_CLK_REG_A27L2_BUSCLKDIV_OFFSET |
				CCU_APP_CLK_REG_A27_MSGBOX_HCLKEN_CLOCK_IS_ON << CCU_APP_CLK_REG_A27_MSGBOX_HCLKEN_OFFSET |
				CCU_APP_CLK_REG_A27L2_CFG_CLKEN_CLOCK_IS_ON << CCU_APP_CLK_REG_A27L2_CFG_CLKEN_OFFSET);

	/* MHCR is T-Head custom CSR 0x7c1. */
	csr_clear(0x7c1, cache_flags);
	udelay(10);
	setbits_le32(ccu_app + BUS_Reset1_REG,
		     BUS_Reset1_REG_A27_RSTN_SW_DE_ASSERT << BUS_Reset1_REG_A27_RSTN_SW_OFFSET | BUS_Reset1_REG_PRESETN_TWI2_SW_DE_ASSERT << BUS_Reset1_REG_PRESETN_TWI2_SW_OFFSET);
	writel((uint32_t)remoteproc->entry, cfg + A27L_START_ADDR_REG);
	writel(0U, cfg + A27L_WFI_MODE_REG);
	setbits_le32(ccu_app + BUS_Reset1_REG, BUS_Reset1_REG_A27_RSTN_SW_DE_ASSERT << BUS_Reset1_REG_A27_RSTN_SW_OFFSET);
	return DRIVER_OK;
}

const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.start = sun300iw1_a27l2_start,
};

DT2C_DRIVER_COMPAT("allwinner,sun300iw1-a27l2");
