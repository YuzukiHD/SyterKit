/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>

#define CPU_UPDATE_OFFSET (26)
#define CPU_LOCK_OFFSET (28)
#define CPU_LOCK_ENABLE_OFFSET (29)

/**
 * @brief Enables the PLL (Phase-Locked Loop) with a given factor.
 *
 * This function configures and enables the PLL by setting specific bits in the
 * provided register address. The PLL configuration parameters such as `n_factor`,
 * lock enable, and update offset are adjusted to match the desired PLL configuration.
 * After configuring, the function waits for the PLL to lock and then performs a
 * final check to ensure the lock is complete.
 *
 * @param reg_addr The register address to configure the PLL.
 * @param n_factor The PLL multiplier factor (used to set the PLL's frequency).
 *                  Typically, this value will be used in the calculation: 
 *                  PLL Frequency = 24M * n / p / (m0 * m1).
 *                  A specific n_factor value adjusts the PLL's output.
 *
 * @note The function waits for the PLL to lock and ensures proper updates and lock status
 *       before completing the operation. Additionally, delays and register bit manipulations
 *       ensure the PLL is stable and functioning.
 */
static void enable_pll(uint32_t reg_addr, uint32_t n_factor) {
	uint32_t reg_val = 0;
	reg_val = readl(reg_addr);
	/* set n=0x28,m0=m1=1,p=1 */
	/* 24M*n/p/(m0 * m1) */
	reg_val &= ~((0xff << 8) | (0x7 << 16) | (0x3 << 20) | (0xf << 0));
	reg_val |= (n_factor << 8) | (0x0 << 0);
	/* lock enable */
	reg_val &= (~(0x1 << CPU_LOCK_ENABLE_OFFSET));
	writel(reg_val, reg_addr);
	reg_val |= (0x1 << CPU_LOCK_ENABLE_OFFSET);
	writel(reg_val, reg_addr);

	/* update_bit */
	reg_val = readl(reg_addr);
	reg_val |= (0x1 << CPU_UPDATE_OFFSET);
	writel(reg_val, reg_addr);
	do {
		reg_val = readl(reg_addr);
		reg_val = reg_val & (0x1 << CPU_UPDATE_OFFSET);
	} while (reg_val);

	udelay(26);
	do {
		reg_val = readl(reg_addr);
		reg_val = reg_val & (0x1 << CPU_LOCK_OFFSET);
	} while (!reg_val);
}

/**
 * @brief Configures the CPU and DSU PLLs, and sets the CPU-to-AXI divider.
 *
 * This function configures the PLLs for the CPU and DSU by calling the `enable_pll`
 * function with predefined PLL multiplier factors. It also adjusts the CPU-to-AXI clock 
 * divider by modifying the corresponding register to control the clock speed and synchronization.
 *
 * @note The function performs PLL configuration for both the CPU (CCU_REG_PLL_C0_CPUX) 
 *       and DSU (CCU_REG_PLL_C0_DSU) and sets the CPU-to-AXI clock divider factor.
 */
static void set_pll_cpux_axi(void) {
	uint32_t reg_val;

	// Enable CPU and DSU PLLs with appropriate factors
	enable_pll(CCU_REG_PLL_C0_CPUX, 0x2a);///< Set the CPU PLL multiplier factor (0x2a)
	enable_pll(CCU_REG_PLL_C0_DSU, 0x16); ///< Set the DSU PLL multiplier factor (0x16)

	/* Set the CPU-to-AXI divider factor (M) */
	reg_val = readl(CCU_REG_DSU_CLK);///< Read the current DSU clock register value
	reg_val &= ~(0x3);				 ///< Clear the divider bits
	writel(reg_val, CCU_REG_DSU_CLK);///< Write the modified value back to the DSU clock register
}

/**
 * @brief Configures the APB clock source and divider.
 *
 * This function configures the APB1 clock source and sets its divider to a default 
 * value by modifying the appropriate control registers. It ensures the APB clock is 
 * sourced from the High-Speed Oscillator (HOSC) and sets the APB clock factor to its 
 * default value.
 *
 * @note This function performs two key actions: setting the APB clock source and 
 *       resetting the APB clock divider factor.
 */
static void set_apb(void) {
	uint32_t reg_value = 0;

	// Set APB1 clock source to HOSC
	reg_value = readl(CCU_APB1_CFG_GREG);											///< Read the current APB1 config register value
	reg_value &= ~APB1_CLK_REG_CLK_SRC_SEL_CLEAR_MASK;								///< Clear the clock source selection mask
	reg_value |= (APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET);///< Set HOSC as the clock source
	writel(reg_value, CCU_APB1_CFG_GREG);											///< Write the modified value back to the APB1 config register
	udelay(10);																		///< Delay to ensure stable configuration

	// Reset the APB clock divider factor to default
	reg_value = readl(CCU_APB1_CFG_GREG);		   ///< Read the APB1 config register again
	reg_value &= ~APB1_CLK_REG_FACTOR_M_CLEAR_MASK;///< Clear the APB divider mask
	writel(reg_value, CCU_APB1_CFG_GREG);		   ///< Write the modified value back to the APB1 config register
	udelay(10);									   ///< Delay to ensure stable configuration
}

/**
 * @brief Configures the NSI (Non-Shared Interface) clock source and gating.
 *
 * This function first disables the NSI clock gating, updates the NSI clock divider, 
 * then sets the NSI clock source to DDR PLL, and finally enables the NSI clock. 
 * It also waits for the updates to complete, ensuring that the clock configurations are applied correctly.
 *
 * @note The function performs two main stages: 
 *       1. Disabling clock gating and updating the NSI divider.
 *       2. Setting the NSI clock source to DDR PLL and enabling the NSI clock.
 */
static void set_pll_nsi(void) {
	uint32_t reg_val;
	uint32_t time_cnt = 0;

	/* Disable NSI clock gating */
	reg_val = readl(CCU_NSI_CLK_GREG);						///< Read the current NSI clock register value
	reg_val &= ~(0x1U << NSI_CLK_REG_NSI_CLK_GATING_OFFSET);///< Disable clock gating
	reg_val &= ~(NSI_CLK_REG_NSI_DIV1_CLEAR_MASK);			///< Clear the divider bits
	reg_val |= (0x5U << NSI_CLK_REG_NSI_DIV1_OFFSET);		///< Set the divider factor to 5
	reg_val |= 1 << NSI_CLK_REG_NSI_UPD_OFFSET;				///< Set the update bit
	writel(reg_val, CCU_NSI_CLK_GREG);						///< Write the new configuration

	// Wait for the clock gating update to complete
	do {
		reg_val = readl(CCU_NSI_CLK_GREG);
		reg_val = reg_val & (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);
		udelay(1);

		if (reg_val && (++time_cnt >= 100000)) {
			printk_debug("nsi clk gating update failed!\n");///< Debug message on failure
			break;
		}
	} while (reg_val);

	time_cnt = 0;
	reg_val = readl(CCU_NSI_CLK_GREG);

	/* Set NSI clock source to DDR PLL */
	reg_val &= ~(NSI_CLK_REG_NSI_CLK_SEL_CLEAR_MASK);							  ///< Clear the clock source selection bits
	reg_val |= (NSI_CLK_REG_NSI_CLK_SEL_DDRPLL << NSI_CLK_REG_NSI_CLK_SEL_OFFSET);///< Set DDR PLL as the clock source

	/* Enable NSI clock */
	reg_val |= (0X01 << NSI_CLK_REG_NSI_CLK_GATING_OFFSET);///< Enable clock gating
	reg_val |= (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);	   ///< Set the update bit
	writel(reg_val, CCU_NSI_CLK_GREG);					   ///< Write the configuration to the register

	// Wait for the clock update to complete
	do {
		reg_val = readl(CCU_NSI_CLK_GREG);
		reg_val = reg_val & (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);
		udelay(1);

		if (reg_val && (++time_cnt >= 100000)) {
			printk_debug("nsi clk update failed!\n");///< Debug message on failure
			break;
		}
	} while (reg_val);
}

/**
 * @brief Configures the MBUS (Memory Bus) clock source and gating.
 *
 * This function first disables the MBUS clock gating, updates the MBUS clock divider, 
 * then sets the MBUS clock source to DDR PLL, and finally enables the MBUS clock. 
 * It waits for the updates to complete to ensure proper clock configuration.
 *
 * @note The function performs two main stages:
 *       1. Disabling clock gating and updating the MBUS divider.
 *       2. Setting the MBUS clock source to DDR PLL and enabling the MBUS clock.
 */
static void set_pll_mbus(void) {
	uint32_t reg_val = 0x0;
	uint32_t time_cnt = 0;

	/* Disable MBUS clock gating */
	reg_val = readl(CCU_MBUS_CFG_REG);						  ///< Read the current MBUS configuration register value
	reg_val &= ~(0x1U << MBUS_CLK_REG_MBUS_CLK_GATING_OFFSET);///< Disable clock gating
	reg_val &= ~(MBUS_CLK_REG_MBUS_DIV1_CLEAR_MASK);		  ///< Clear the divider bits
	reg_val |= (0x5U << MBUS_CLK_REG_MBUS_DIV1_OFFSET);		  ///< Set the divider factor to 5
	reg_val |= 1 << MBUS_CLK_REG_MBUS_UPD_OFFSET;			  ///< Set the update bit
	writel(reg_val, CCU_MBUS_CFG_REG);						  ///< Write the configuration to the MBUS register

	// Wait for the clock gating update to complete
	do {
		reg_val = readl(CCU_MBUS_CFG_REG);
		reg_val = reg_val & (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);
		udelay(1);

		if (reg_val && (++time_cnt >= 100000)) {
			printk_debug("mbus clk gating update failed!\n");///< Debug message on failure
			break;
		}
	} while (reg_val);

	time_cnt = 0;

	reg_val = readl(CCU_MBUS_CFG_REG);

	/* Set MBUS clock source to DDR PLL */
	reg_val &= ~(MBUS_CLK_REG_MBUS_CLK_SEL_CLEAR_MASK);								  ///< Clear the clock source selection bits
	reg_val |= (MBUS_CLK_REG_MBUS_CLK_SEL_DDRPLL << MBUS_CLK_REG_MBUS_CLK_SEL_OFFSET);///< Set DDR PLL as the clock source

	/* Enable MBUS clock */
	reg_val |= (0X01 << MBUS_CLK_REG_MBUS_CLK_GATING_OFFSET);///< Enable clock gating
	reg_val |= (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);		 ///< Set the update bit
	writel(reg_val, CCU_MBUS_CFG_REG);						 ///< Write the configuration to the MBUS register

	// Wait for the clock update to complete
	do {
		reg_val = readl(CCU_MBUS_CFG_REG);
		reg_val = reg_val & (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);
		udelay(1);

		if (reg_val && (++time_cnt >= 100000)) {
			printk_debug("mbus clk update failed!\n");///< Debug message on failure
			break;
		}
	} while (reg_val);
}

/**
 * @brief Initializes the clocks for the Sunxi platform.
 * 
 * This function configures various clock sources for the CPU, AXI, APB,
 * NSI, and MBUS based on the Sunxi platform's requirements. The function
 * also prints debug messages to track the initialization process.
 * 
 * It performs the following tasks:
 * - Configures the CPU and AXI clock using the `set_pll_cpux_axi()` function.
 * - Configures the APB clock using the `set_apb()` function.
 * - Configures the NSI clock using the `set_pll_nsi()` function.
 * - Configures the MBUS clock using the `set_pll_mbus()` function.
 * 
 * @note This function should be called during the system initialization
 *       process to ensure the correct operation of the platform's clock system.
 * 
 * @warning Ensure that all the clock configuration functions are correctly
 *          implemented and tested to avoid system instability.
 */
void sunxi_clk_init(void) {
	printk_debug("Set pll start\n");

	// Set the CPU and AXI clock via the set_pll_cpux_axi function
	set_pll_cpux_axi();

	// Set the APB clock using the set_apb function
	set_apb();

	// Set the NSI clock (Non-Shared Interface) via the set_pll_nsi function
	set_pll_nsi();

	// Set the MBUS clock (Memory Bus) using the set_pll_mbus function
	set_pll_mbus();

	printk_debug("Set pll end\n");
}

void sunxi_clk_dump() {
	uint32_t reg32;
	uint32_t cpu_clk_src;
	uint32_t plln, pllm;
	uint8_t p0;
	uint8_t p1;

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCU_BASE + PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		printk_debug("CLK: PLL_PERI0 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		printk_debug("CLK: PLL_PERI0 disabled\r\n");
	}

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCU_BASE + PLL_PERI1_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		printk_debug("CLK: PLL_PERI1 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		printk_debug("CLK: PLL_PERI1 disabled\r\n");
	}

	/* PLL DDR */
	reg32 = read32(SUNXI_CCU_BASE + PLL_DDR_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		printk_debug("CLK: PLL_DDR1=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		printk_debug("CLK: PLL_DDR1 disabled\r\n");
	}
}
