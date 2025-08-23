/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>

#include <sys-rproc.h>

/**
 * @brief Boot the A27L2 processor.
 * 
 * This function configures clocks, reset controls, and cache settings to boot the A27L processor.
 * It performs a series of hardware register operations to ensure the processor is in the correct state 
 * during boot-up.
 * 
 * @param addr The address to start the A27L processor. This is typically the entry point of a program or firmware.
 * 
 * @note 
 * - This function disables interrupts on entry and performs necessary clock and reset settings before 
 *   booting the A27L2 processor.
 * - Ensure that relevant registers and addresses are properly initialized before calling this function.
 * - The function uses assembly instructions to directly manipulate hardware registers, thus requiring 
 *   the compiler to understand the embedded assembly code.
 * 
 * @warning 
 * - Make sure the relevant hardware state is ready before invoking this function to avoid undefined behavior.
 */
void sunxi_ansc_boot(uint32_t addr) {
	uint32_t reg_val = 0;

	/* Disable IRQ (Interrupt Request) */
	asm volatile("csrc mstatus, 0x8");

	/* Enable wake-up control for A27L */
	setbits_le32(SUNXI_WAKUP_CTRL_REG, SUNXI_A27L_WAKUP_EN);

	/* Enable A27L2 clock */
	writel(A27L2_CLK_REG_A27L2_CLK_EN_CLOCK_IS_ON << A27L2_CLK_REG_A27L2_CLK_EN_OFFSET, CCU_A27_CLK_REG);

	/* Enable MT clock for A27L2 */
	writel(CCU_A27L2_MTCLK_EN, SUNXI_CCU_APP_BASE + CCU_A27L2_MTCLK_REG);

	/* Configure clock division and enable necessary clocks */
	clrsetbits_le32((SUNXI_CCU_APP_BASE + CCU_APP_CLK_REG), CCU_APP_CLK_REG_A27L2_BUSCLKDIV_CLEAR_MASK,
					CCU_APP_CLK_REG_A27L2_BUSCLKDIV_DIV2 << CCU_APP_CLK_REG_A27L2_BUSCLKDIV_OFFSET |
							CCU_APP_CLK_REG_A27_MSGBOX_HCLKEN_CLOCK_IS_ON << CCU_APP_CLK_REG_A27_MSGBOX_HCLKEN_OFFSET |
							CCU_APP_CLK_REG_A27L2_CFG_CLKEN_CLOCK_IS_ON << CCU_APP_CLK_REG_A27L2_CFG_CLKEN_OFFSET);

	/* Disable Instruction Cache */
	asm volatile("li %0, 0x103f\n" /* Load immediate value 0x103f into flags */
				 "csrc mhcr, %0"   /* Clear the specified bits in mhcr register */
				 :
				 : "r"(reg_val));

	/* Delay for 10 microseconds */
	udelay(10);

	/* Assert the reset for A27 and TWI2 */
	setbits_le32((SUNXI_CCU_APP_BASE + BUS_Reset1_REG),
				 BUS_Reset1_REG_A27_RSTN_SW_DE_ASSERT << BUS_Reset1_REG_A27_RSTN_SW_OFFSET | BUS_Reset1_REG_PRESETN_TWI2_SW_DE_ASSERT << BUS_Reset1_REG_PRESETN_TWI2_SW_OFFSET);

	/* Write the start address to the A27L start address register */
	writel(addr, SUNXI_A27L_START_ADD_REG);

	/* Set WFI (Wait For Interrupt) mode */
	writel(0x0, SUNXI_A27L_WFI_MODE_REG);

	/* Wait until the start address register indicates the processor is ready */
	while (readl(SUNXI_A27L_START_ADD_REG) == 0x0)
		;

	/* De-assert reset for A27 */
	setbits_le32((SUNXI_CCU_APP_BASE + BUS_Reset1_REG), BUS_Reset1_REG_A27_RSTN_SW_DE_ASSERT << BUS_Reset1_REG_A27_RSTN_SW_OFFSET);
}
