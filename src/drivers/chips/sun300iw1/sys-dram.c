/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <sys-clk.h>
#include <sys-dram.h>

#include <common.h>

#define DIV_ROUND_UP(a, b) (((a) + (b) -1) / (b))

#define CONFIG_SYS_SDRAM_BASE SDRAM_BASE

/**
 * @brief Convert nanoseconds to clock cycles for DRAM timing.
 *
 * This function calculates the corresponding number of clock cycles for a given 
 * duration in nanoseconds, based on the DRAM controller clock frequency.
 *
 * @param para Pointer to a struct containing DRAM parameters, including the clock frequency.
 * @param nanoseconds The time duration in nanoseconds to be converted.
 * 
 * @return The equivalent time in clock cycles (rounded up) for the given duration.
 */
static int ns_to_t(dram_para_t *para, int nanoseconds) {
	const unsigned int ctrl_freq = para->dram_clk / 2;

	// Return the time in clock cycles, rounded up
	return DIV_ROUND_UP(ctrl_freq * nanoseconds, 1000);
}

/**
 * @brief Enable all DRAM masters.
 *
 * This function enables all masters in the DRAM controller by writing to the 
 * appropriate registers. It also introduces a small delay to ensure the changes 
 * are applied.
 */
static void dram_enable_all_master(void) {
	writel(~0, (MCTL_COM_BASE + MCTL_COM_MAER0));	 // Enable all masters in MAER0
	writel(0xff, (MCTL_COM_BASE + MCTL_COM_MAER1));	 // Enable all masters in MAER1
	writel(0xffff, (MCTL_COM_BASE + MCTL_COM_MAER2));// Enable all masters in MAER2
	udelay(10);										 // Delay for 10 microseconds to allow changes to take effect
}

/**
 * @brief Disable all DRAM masters.
 *
 * This function disables all masters in the DRAM controller by writing to the 
 * appropriate registers. It also introduces a small delay to ensure the changes 
 * are applied.
 */
static void dram_disable_all_master(void) {
	writel(1, (MCTL_COM_BASE + MCTL_COM_MAER0));// Disable all masters in MAER0
	writel(0, (MCTL_COM_BASE + MCTL_COM_MAER1));// Disable all masters in MAER1
	writel(0, (MCTL_COM_BASE + MCTL_COM_MAER2));// Disable all masters in MAER2
	udelay(10);									// Delay for 10 microseconds to allow changes to take effect
}

/**
 * @brief Perform eye delay compensation for DRAM timings.
 *
 * This function configures various delay parameters for DRAM by manipulating 
 * specific PHY (Physical Layer) control registers. The compensation ensures 
 * that the timing of various signal transitions (e.g., DQS, RAS, CAS) is optimized 
 * for the DRAM device, based on the provided timing parameters.
 * The adjustments are based on values extracted from the `dram_para_t` structure.
 * The function configures the appropriate delay settings for data signals, command 
 * signals, and other necessary PHY control registers.
 *
 * @param para Pointer to a structure containing the DRAM timing parameters. These 
 *             parameters are used to configure delays for various signal lines 
 *             in the DRAM controller.
 */
static void eye_delay_compensation(dram_para_t *para)// s1
{
	uint32_t delay, i = 0;

	// DATn0IOCR, n =  0...7 - Configure delay for DATn0 I/O control registers
	delay = (para->dram_tpr11 & 0xf) << 9; // Extract and adjust delay from dram_tpr11
	delay |= (para->dram_tpr12 & 0xf) << 1;// Extract and adjust delay from dram_tpr12
	for (i = 0; i < 9; i++) setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(i)), delay);

	// DATn1IOCR, n =  0...7 - Configure delay for DATn1 I/O control registers
	delay = (para->dram_tpr11 & 0xf0) << 5; // Extract and adjust delay from dram_tpr11
	delay |= (para->dram_tpr12 & 0xf0) >> 3;// Extract and adjust delay from dram_tpr12
	for (i = 0; i < 9; i++) setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(i)), delay);

	// PGCR0: Assert AC loopback FIFO reset
	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), 0x04000000);

	// DQS0 read and write delay configuration
	delay = (para->dram_tpr11 & 0xf0000) >> 7;					  // Extract delay from dram_tpr11
	delay |= (para->dram_tpr12 & 0xf0000) >> 15;				  // Extract delay from dram_tpr12
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(9)), delay); // Configure DQS0 positive delay
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(10)), delay);// Configure DQS0 negative delay

	// DQS1 read and write delay configuration
	delay = (para->dram_tpr11 & 0xf00000) >> 11;				  // Extract delay from dram_tpr11
	delay |= (para->dram_tpr12 & 0xf00000) >> 19;				  // Extract delay from dram_tpr12
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(9)), delay); // Configure DQS1 positive delay
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(10)), delay);// Configure DQS1 negative delay

	// DQS0 enable bit delay configuration
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnSDLR6(0)), (para->dram_tpr11 & 0xf0000) << 9);

	// DQS1 enable bit delay configuration
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnSDLR6(1)), (para->dram_tpr11 & 0xf00000) << 5);

	// PGCR0: Release AC loopback FIFO reset
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), (1 << 26));

	// Wait for 1 microsecond to ensure changes take effect
	udelay(1);

	// Set RAS, CAS, and CA delay for DRAM timing
	delay = (para->dram_tpr10 & 0xf0) << 4;// Extract delay from dram_tpr10
	for (i = 6; i < 27; ++i) {
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(i)), delay);// Apply delay to AC IO control registers
	}

	// Set CK CS delay based on dram_tpr10
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(2)), (para->dram_tpr10 & 0x0f) << 8);
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(3)), (para->dram_tpr10 & 0x0f) << 8);
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(28)), (para->dram_tpr10 & 0xf00) >> 4);
}

/**
 * @brief Set the DRAM timing parameters for the specified DRAM type.
 * 
 * This function configures various timing parameters for the DRAM controller
 * based on the DRAM type and clock speed specified in the `dram_para_t` structure.
 * It sets the timing values for the DRAM and writes them to the appropriate registers.
 * The function supports multiple DRAM types, including DDR2, DDR3, LPDDR2, and LPDDR3.
 * 
 * The following parameters are configured:
 * - DRAM timing values (e.g., tCCD, tFAW, tRCD, tRP, tCAS latency, etc.)
 * - Mode register values (mr0, mr1, mr2, mr3)
 * - Initialization timing (e.g., tdinit0, tdinit1, tdinit2, tdinit3)
 * 
 * @param para Pointer to a structure that contains the DRAM parameters, including:
 *             - dram_type: The type of DRAM (e.g., DDR2, DDR3, LPDDR2, LPDDR3).
 *             - dram_clk: The DRAM clock frequency in MHz.
 *             - dram_mr1: Value to be written to the MR1 register.
 *             - Other parameters related to the DRAM configuration.
 * 
 * @note The function uses the `ns_to_t` helper function to convert nanosecond values
 *       to clock cycles based on the DRAM clock frequency.
 *       The DRAM type and clock speed influence the specific values of the timing parameters.
 * 
 * @return None
 */
static void mctl_set_timing_params(dram_para_t *para) {
	/* DRAM_TPR0 */
	uint8_t tccd = 2;
	uint8_t tfaw;
	uint8_t trrd;
	uint8_t trcd;
	uint8_t trc;

	/* DRAM_TPR1 */
	uint8_t txp;
	uint8_t twtr;
	uint8_t trtp = 4;
	uint8_t twr;
	uint8_t trp;
	uint8_t tras;

	/* DRAM_TPR2 */
	uint16_t trefi;
	uint16_t trfc;

	uint8_t tcksrx;
	uint8_t tckesr;
	uint8_t trd2wr;
	uint8_t twr2rd;
	uint8_t trasmax;
	uint8_t twtp;
	uint8_t tcke;
	uint8_t tmod;
	uint8_t tmrd;
	uint8_t tmrw;

	uint8_t tcl;
	uint8_t tcwl;
	uint8_t t_rdata_en;
	uint8_t wr_latency;

	uint32_t mr0;
	uint32_t mr1;
	uint32_t mr2;
	uint32_t mr3;

	uint32_t tdinit0;
	uint32_t tdinit1;
	uint32_t tdinit2;
	uint32_t tdinit3;

	switch (para->dram_type) {
		case SUNXI_DRAM_TYPE_DDR2:
			/* DRAM_TPR0 */
			tfaw = ns_to_t(para, 50);
			trrd = ns_to_t(para, 10);
			trcd = ns_to_t(para, 20);
			trc = ns_to_t(para, 65);

			/* DRAM_TPR1 */
			txp = 2;
			twtr = ns_to_t(para, 8);
			twr = ns_to_t(para, 15);
			trp = ns_to_t(para, 15);
			tras = ns_to_t(para, 45);

			/* DRAM_TRP2 */
			trfc = ns_to_t(para, 328);
			trefi = ns_to_t(para, 7800) / 32;

			trasmax = para->dram_clk / 30;
			if (para->dram_clk < 409) {
				t_rdata_en = 1;
				tcl = 3;
				mr0 = 0x06a3;
			} else {
				t_rdata_en = 2;
				tcl = 4;
				mr0 = 0x0e73;
			}
			tmrd = 2;
			twtp = twr + 5;
			tcksrx = 5;
			tckesr = 4;
			trd2wr = 4;
			tcke = 3;
			tmod = 12;
			wr_latency = 1;
			tmrw = 0;
			twr2rd = twtr + 5;
			tcwl = 0;

			mr1 = para->dram_mr1;
			mr2 = 0;
			mr3 = 0;

			tdinit0 = 200 * para->dram_clk + 1;
			tdinit1 = 100 * para->dram_clk / 1000 + 1;
			tdinit2 = 200 * para->dram_clk + 1;
			tdinit3 = 1 * para->dram_clk + 1;

			break;
		case SUNXI_DRAM_TYPE_DDR3:
			trfc = ns_to_t(para, 350);
			trefi = ns_to_t(para, 7800) / 32 + 1;// XXX

			twtr = ns_to_t(para, 8) + 2;// + 2 ? XXX
			/* Only used by trd2wr calculation, which gets discard below */
			//		twr		= max(ns_to_t(para, 15), 2);
			trrd = max(ns_to_t(para, 10), 2);
			txp = max(ns_to_t(para, 10), 2);

			if (para->dram_clk <= 800) {
				tfaw = ns_to_t(para, 50);
				trcd = ns_to_t(para, 15);
				trp = ns_to_t(para, 15);
				trc = ns_to_t(para, 53);
				tras = ns_to_t(para, 38);

				mr0 = 0x1c70;
				mr2 = 0x18;
				tcl = 6;
				wr_latency = 2;
				tcwl = 4;
				t_rdata_en = 4;
			} else {
				tfaw = ns_to_t(para, 35);
				trcd = ns_to_t(para, 14);
				trp = ns_to_t(para, 14);
				trc = ns_to_t(para, 48);
				tras = ns_to_t(para, 34);

				mr0 = 0x1e14;
				mr2 = 0x20;
				tcl = 7;
				wr_latency = 3;
				tcwl = 5;
				t_rdata_en = 5;
			}

			trasmax = para->dram_clk / 30;
			twtp = tcwl + 2 + twtr;// WL+BL/2+tWTR
			/* Gets overwritten below */
			//		trd2wr		= tcwl + 2 + twr;		// WL+BL/2+tWR
			twr2rd = tcwl + twtr;// WL+tWTR

			tdinit0 = 500 * para->dram_clk + 1;		  // 500 us
			tdinit1 = 360 * para->dram_clk / 1000 + 1;// 360 ns
			tdinit2 = 200 * para->dram_clk + 1;		  // 200 us
			tdinit3 = 1 * para->dram_clk + 1;		  //   1 us

			mr1 = para->dram_mr1;
			mr3 = 0;
			tcke = 3;
			tcksrx = 5;
			tckesr = 4;
			if (((para->dram_tpr13 & 0xc) == 0x04) || para->dram_clk < 912)
				trd2wr = 5;
			else
				trd2wr = 6;

			tmod = 12;
			tmrd = 4;
			tmrw = 0;

			break;
		case SUNXI_DRAM_TYPE_LPDDR2:
			tfaw = max(ns_to_t(para, 50), 4);
			trrd = max(ns_to_t(para, 10), 1);
			trcd = max(ns_to_t(para, 24), 2);
			trc = ns_to_t(para, 70);
			txp = ns_to_t(para, 8);
			if (txp < 2) {
				txp++;
				twtr = 2;
			} else {
				twtr = txp;
			}
			twr = max(ns_to_t(para, 15), 2);
			trp = ns_to_t(para, 17);
			tras = ns_to_t(para, 42);
			trefi = ns_to_t(para, 3900) / 32;
			trfc = ns_to_t(para, 210);

			trasmax = para->dram_clk / 60;
			mr3 = para->dram_mr3;
			twtp = twr + 5;
			mr2 = 6;
			mr1 = 5;
			tcksrx = 5;
			tckesr = 5;
			trd2wr = 10;
			tcke = 2;
			tmod = 5;
			tmrd = 5;
			tmrw = 3;
			tcl = 4;
			wr_latency = 1;
			t_rdata_en = 1;

			tdinit0 = 200 * para->dram_clk + 1;
			tdinit1 = 100 * para->dram_clk / 1000 + 1;
			tdinit2 = 11 * para->dram_clk + 1;
			tdinit3 = 1 * para->dram_clk + 1;
			twr2rd = twtr + 5;
			tcwl = 2;
			mr1 = 195;
			mr0 = 0;

			break;
		case SUNXI_DRAM_TYPE_LPDDR3:
			tfaw = max(ns_to_t(para, 50), 4);
			trrd = max(ns_to_t(para, 10), 1);
			trcd = max(ns_to_t(para, 24), 2);
			trc = ns_to_t(para, 70);
			twtr = max(ns_to_t(para, 8), 2);
			twr = max(ns_to_t(para, 15), 2);
			trp = ns_to_t(para, 17);
			tras = ns_to_t(para, 42);
			trefi = ns_to_t(para, 3900) / 32;
			trfc = ns_to_t(para, 210);
			txp = twtr;

			trasmax = para->dram_clk / 60;
			if (para->dram_clk < 800) {
				tcwl = 4;
				wr_latency = 3;
				t_rdata_en = 6;
				mr2 = 12;
			} else {
				tcwl = 3;
				tcke = 6;
				wr_latency = 2;
				t_rdata_en = 5;
				mr2 = 10;
			}
			twtp = tcwl + 5;
			tcl = 7;
			mr3 = para->dram_mr3;
			tcksrx = 5;
			tckesr = 5;
			trd2wr = 13;
			tcke = 3;
			tmod = 12;
			tdinit0 = 400 * para->dram_clk + 1;
			tdinit1 = 500 * para->dram_clk / 1000 + 1;
			tdinit2 = 11 * para->dram_clk + 1;
			tdinit3 = 1 * para->dram_clk + 1;
			tmrd = 5;
			tmrw = 5;
			twr2rd = tcwl + twtr + 5;
			mr1 = 195;
			mr0 = 0;

			break;
		default:
			trfc = 128;
			trp = 6;
			trefi = 98;
			txp = 10;
			twr = 8;
			twtr = 3;
			tras = 14;
			tfaw = 16;
			trc = 20;
			trcd = 6;
			trrd = 3;

			twr2rd = 8;	   // 48(sp)
			tcksrx = 4;	   // t1
			tckesr = 3;	   // t4
			trd2wr = 4;	   // t6
			trasmax = 27;  // t3
			twtp = 12;	   // s6
			tcke = 2;	   // s8
			tmod = 6;	   // t0
			tmrd = 2;	   // t5
			tmrw = 0;	   // a1
			tcwl = 3;	   // a5
			tcl = 3;	   // a0
			wr_latency = 1;// a7
			t_rdata_en = 1;// a4
			mr3 = 0;	   // s0
			mr2 = 0;	   // t2
			mr1 = 0;	   // s1
			mr0 = 0;	   // a3
			tdinit3 = 0;   // 40(sp)
			tdinit2 = 0;   // 32(sp)
			tdinit1 = 0;   // 24(sp)
			tdinit0 = 0;   // 16(sp)

			break;
	}

	/* Set mode registers */
	writel(mr0, (MCTL_PHY_BASE + MCTL_PHY_DRAM_MR0));
	writel(mr1, (MCTL_PHY_BASE + MCTL_PHY_DRAM_MR1));
	writel(mr2, (MCTL_PHY_BASE + MCTL_PHY_DRAM_MR2));
	writel(mr3, (MCTL_PHY_BASE + MCTL_PHY_DRAM_MR3));
	writel((para->dram_odt_en >> 4) & 0x3, (MCTL_PHY_BASE + MCTL_PHY_LP3MR11));

	/* Set dram timing DRAMTMG0 - DRAMTMG5 */
	writel((twtp << 24) | (tfaw << 16) | (trasmax << 8) | (tras << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG0));
	writel((txp << 16) | (trtp << 8) | (trc << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG1));
	writel((tcwl << 24) | (tcl << 16) | (trd2wr << 8) | (twr2rd << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG2));
	writel((tmrw << 16) | (tmrd << 12) | (tmod << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG3));
	writel((trcd << 24) | (tccd << 16) | (trrd << 8) | (trp << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG4));
	writel((tcksrx << 24) | (tcksrx << 16) | (tckesr << 8) | (tcke << 0), (MCTL_PHY_BASE + MCTL_PHY_DRAMTMG5));

	/* Set dual rank timing */
	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DRAMTMG8), 0xf000ffff, (para->dram_clk < 800) ? 0xf0006610 : 0xf0007610);

	/* Set phy interface time PITMG0, PTR3, PTR4 */
	writel((0x2 << 24) | (t_rdata_en << 16) | (1 << 8) | (wr_latency << 0), (MCTL_PHY_BASE + MCTL_PHY_PITMG0));
	writel(((tdinit0 << 0) | (tdinit1 << 20)), (MCTL_PHY_BASE + MCTL_PHY_PTR3));
	writel(((tdinit2 << 0) | (tdinit3 << 20)), (MCTL_PHY_BASE + MCTL_PHY_PTR4));

	/* Set refresh timing and mode */
	writel((trefi << 16) | (trfc << 0), (MCTL_PHY_BASE + MCTL_PHY_RFSHTMG));
	writel((trefi << 15) & 0x0fff0000, (MCTL_PHY_BASE + MCTL_PHY_RFSHCTL1));
}

/**
 * @brief Set the DDR PLL clock based on the given DRAM parameters.
 * 
 * This function configures the PLL clock for the DDR memory controller based on 
 * the DRAM parameters passed in the `dram_para_t` structure. It calculates the
 * PLL parameters (such as the PLL multiplier and divider) and writes them to the
 * appropriate registers to enable the PLL and ensure it locks to the desired frequency.
 * It also configures the DRAM clock source to use the PLL.
 * 
 * The PLL is configured using the following parameters:
 * - PLL N (PLL multiplier) 
 * - PLL M0, M1 (input/output dividers)
 * - PLL enable and lock control
 * - Gate control for the PLL output
 * 
 * The function uses hardware registers to configure the PLL and waits for the PLL
 * to lock before enabling the output gate and selecting the PLL as the clock source
 * for the DRAM controller.
 * 
 * @param index The index used to determine which PLL clock configuration to use.
 *              This is typically used to select different PLL configurations 
 *              based on the input parameters.
 * @param para A pointer to a structure containing the DRAM parameters, including:
 *             - dram_clk: The DRAM clock frequency in MHz.
 *             - dram_tpr13: Contains information for selecting PLL configurations.
 *             - dram_tpr9: Used for PLL clock calculation.
 *             - dram_tpr10: Contains high-speed oscillator frequency for PLL calculation.
 * 
 * @return The resulting DDR PLL frequency in Hz, calculated as:
 *         \[
 *         \text{hosc\_freq} \times n / p0 / m0 / m1
 *         \]
 *         Where:
 *         - \(\text{hosc\_freq}\) is the high-speed oscillator frequency.
 *         - \(n\) is the PLL multiplier.
 *         - \(p0, m0, m1\) are divider values for the PLL.
 * 
 * @note The PLL frequency calculation takes into account the DRAM clock, dividers,
 *       and the high-speed oscillator frequency (`hosc_freq`). 
 *       The function assumes that `dram_tpr13`, `dram_tpr9`, and `dram_tpr10`
 *       contain the necessary information for PLL configuration.
 * 
 * @note This function performs low-level hardware register writes to configure the PLL
 *       and DRAM clock. It directly interacts with the hardware, so it's intended for
 *       use in a low-level driver or platform-specific initialization code.
 * 
 * @note The function waits for the PLL to lock, which involves polling a lock status
 *       bit in the PLL control register. It then enables the output gate to allow the
 *       PLL to drive the DRAM clock.
 * 
 * @note Ensure that the values in the `dram_para_t` structure are valid and appropriate
 *       for your system configuration before calling this function.
 */
static int ccu_set_pll_ddr_clk(int index, dram_para_t *para) {
	uint32_t reg_val = 0, n = 12, m1 = 1, p0 = 1, m0 = 1, pll_clk, hosc_freq;

	if (((para->dram_tpr13 >> 6) & 0x1) == index)
		pll_clk = (para->dram_clk << 1);
	else
		pll_clk = (para->dram_tpr9 << 1);

	hosc_freq = (para->dram_tpr10 >> 16) & 0xff;
	printk_debug("DRAM set hosc_freq = 0x%x\n", hosc_freq);
	n = pll_clk * m0 * m1 / hosc_freq;
	if (n < 12) {
		n = n * 4;
		m1 = 2;
		m0 = 2;
	}

	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG,
					PLL_DDR_CTRL_REG_PLL_N_CLEAR_MASK | PLL_DDR_CTRL_REG_PLL_OUTPUT_DIV2_CLEAR_MASK | PLL_DDR_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK |
							PLL_DDR_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
					(PLL_DDR_CTRL_REG_PLL_EN_ENABLE << PLL_DDR_CTRL_REG_PLL_EN_OFFSET) | (PLL_DDR_CTRL_REG_PLL_LDO_EN_ENABLE << PLL_DDR_CTRL_REG_PLL_LDO_EN_OFFSET) |
							((n - 1) << PLL_DDR_CTRL_REG_PLL_N_OFFSET) | ((m1 - 1) << PLL_DDR_CTRL_REG_PLL_INPUT_DIV_OFFSET) |
							((m0 - 1) << PLL_DDR_CTRL_REG_PLL_OUTPUT_DIV2_OFFSET));

	/* Clear PLL Lock */
	reg_val = readl(SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);
	reg_val &= ~PLL_DDR_CTRL_REG_LOCK_ENABLE_CLEAR_MASK;
	writel(reg_val, SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);

	/* Set PLL Lock */
	reg_val = readl(SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);
	writel(reg_val | (PLL_DDR_CTRL_REG_LOCK_ENABLE_ENABLE << PLL_DDR_CTRL_REG_LOCK_ENABLE_OFFSET), SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);

	/* Wait PLL Lock */
	while (!(readl(SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG) & PLL_DDR_CTRL_REG_LOCK_LOCKED__IT_INDICATES_THAT_THE_PLL_HAS_BEEN_STABLE << PLL_DDR_CTRL_REG_LOCK_OFFSET))
		;
	udelay(20);

	/* Set Gate Open */
	reg_val = readl(SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);
	reg_val |= (PLL_DDR_CTRL_REG_PLL_OUTPUT_GATE_ENABLE << PLL_DDR_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
	writel(reg_val, SUNXI_CCU_AON_BASE + PLL_DDR_CTRL_REG);


	reg_val = readl((SUNXI_CCU_APP_BASE + DRAM_CLK_REG));
	reg_val &= ~DRAM_CLK_REG_DRAM_CLK_SEL_CLEAR_MASK;
	reg_val &= ~(DRAM_CLK_REG_DRAM_DIV1_CLEAR_MASK | DRAM_CLK_REG_DRAM_DIV2_CLEAR_MASK);
	reg_val |= ((DRAM_CLK_REG_DRAM_CLK_GATING_CLOCK_IS_ON << DRAM_CLK_REG_DRAM_CLK_GATING_OFFSET) | (DRAM_CLK_REG_DRAM_CLK_SEL_DDRPLL << DRAM_CLK_REG_DRAM_CLK_SEL_OFFSET));
	writel(reg_val, (SUNXI_CCU_APP_BASE + DRAM_CLK_REG));

	return ((hosc_freq * n) / p0 / m0 / m1);
}

/**
 * @brief Initializes the MCTL (Memory Controller) system by configuring the DRAM and MBUS clocks and resets.
 * 
 * This function initializes the Memory Controller by performing a sequence of hardware register writes 
 * to assert and deassert resets, configure clock gating, and enable the necessary clock sources for DRAM
 * operation. It configures the DRAM clock PLL and updates the clock settings based on the system's 
 * oscillator frequency (HOSC). Additionally, it enables the MCTL clock and prepares the system for 
 * further memory operations.
 * 
 * The steps performed by this function include:
 * 1. Asserting and deasserting the MBUS and DRAM resets.
 * 2. Configuring the MBUS and DRAM clock gating.
 * 3. Setting the DRAM clock PLL frequency and enabling the clock for DRAM operations.
 * 4. Enabling the MCTL clock for PHY access.
 * 
 * This function directly manipulates hardware registers for clock configuration, reset control, and 
 * memory initialization. It's intended for low-level hardware initialization and should be executed 
 * early in the system's startup sequence.
 * 
 * @param para A pointer to a structure of type `dram_para_t` which contains the DRAM configuration 
 *             parameters, including the DRAM clock frequency and PLL settings.
 * 
 * @note This function assumes that the hardware addresses for the clock and reset control registers 
 *       are correct and the system is ready for memory controller configuration.
 * 
 * @note The function uses `udelay` to introduce delays, which might not be precise on all platforms. 
 *       Ensure that the delays are sufficient for your platform's clock configuration to take effect.
 * 
 * @note The `dram_tpr10` register in `para` is modified to configure the HOSC frequency based on 
 *       whether the system is using a 40 MHz or a different oscillator frequency.
 * 
 * @note The `ccu_set_pll_ddr_clk` function is called to configure the PLL for DRAM based on the 
 *       passed parameters. It returns the actual DRAM frequency, which is stored in `para->dram_clk`.
 * 
 * @note This function performs critical operations on the system's memory and clocking hardware and 
 *       should be executed with care, particularly when the system is in the early stages of booting.
 */
static void mctl_sys_init(dram_para_t *para) {
	uint32_t reg_val = 0;

	/* Assert MBUS reset */
	reg_val = readl(SUNXI_CCU_APP_BASE + BUS_Reset1_REG);
	reg_val &= ~BUS_Reset1_REG_MBUS_RSTN_SW_CLEAR_MASK;
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_Reset1_REG));

	/* Close MBUS gate */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG));
	reg_val &= ~BUS_CLK_GATING1_REG_MBUS_GATE_SW_CLEAR_MASK;
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG));

	/* Assert DRAM reset */
	reg_val = readl(SUNXI_CCU_APP_BASE + BUS_Reset0_REG);
	reg_val &= ~BUS_Reset0_REG_DRAM_CLEAR_MASK;
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_Reset0_REG));

	/* Close DRAM gate */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG));
	reg_val &= ~BUS_CLK_GATING0_REG_DRAM_GATING_CLEAR_MASK;
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG));

	/* Close DRAM clock gating */
	reg_val = readl(SUNXI_CCU_APP_BASE + DRAM_CLK_REG);
	reg_val &= ~DRAM_CLK_REG_DRAM_CLK_GATING_CLEAR_MASK;
	writel(reg_val, (SUNXI_CCU_APP_BASE + DRAM_CLK_REG));

	/* Update DRAM clock configuration */
	reg_val = readl(SUNXI_CCU_APP_BASE + DRAM_CLK_REG);
	reg_val |= (DRAM_CLK_REG_DRAM_UPD_VALID << DRAM_CLK_REG_DRAM_UPD_OFFSET);
	writel(reg_val, (SUNXI_CCU_APP_BASE + DRAM_CLK_REG));
	udelay(10);

	/* Adjust HOSC frequency based on oscillator type */
	if (sunxi_clk_get_hosc_type() == HOSC_FREQ_40M) {
		para->dram_tpr10 |= (0x28 << 16);// Set for 40MHz HOSC
	} else {
		para->dram_tpr10 |= (0x18 << 16);// Set for other frequencies
	}

	/* Set PLL for DDR clock */
	reg_val = ccu_set_pll_ddr_clk(0, para);
	printk_debug("CLK: DRAM FREQ = %dMHz\n", reg_val);
	para->dram_clk = reg_val / 2;// Store the actual DRAM clock frequency

	/* Disable all DRAM masters (if any) */
	dram_disable_all_master();

	/* Deassert DRAM reset */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_Reset0_REG));
	reg_val |= (BUS_Reset0_REG_DRAM_DE_ASSERT << BUS_Reset0_REG_DRAM_OFFSET);
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_Reset0_REG));

	/* Deassert MBUS reset */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_Reset1_REG));
	reg_val |= (BUS_Reset1_REG_MBUS_RSTN_SW_DE_ASSERT << BUS_Reset1_REG_MBUS_RSTN_SW_OFFSET);
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_Reset1_REG));

	/* Open DRAM clock gating */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG));
	reg_val |= (BUS_CLK_GATING0_REG_DRAM_GATING_CLOCK_IS_ON << BUS_CLK_GATING0_REG_DRAM_GATING_OFFSET);
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG));

	/* Open MBUS clock gate */
	reg_val = readl((SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG));
	reg_val |= (BUS_CLK_GATING1_REG_MBUS_GATE_SW_CLOCK_IS_ON << BUS_CLK_GATING1_REG_MBUS_GATE_SW_OFFSET);
	writel(reg_val, (SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG));

	/* Open DRAM controller clock gating */
	reg_val = readl(SUNXI_CCU_APP_BASE + DRAM_CLK_REG);
	reg_val |= (DRAM_CLK_REG_DRAM_CLK_GATING_CLOCK_IS_ON << DRAM_CLK_REG_DRAM_CLK_GATING_OFFSET);
	writel(reg_val, SUNXI_CCU_APP_BASE + DRAM_CLK_REG);

	/* Update DRAM clock again after enabling */
	reg_val = readl(SUNXI_CCU_APP_BASE + DRAM_CLK_REG);
	reg_val |= (DRAM_CLK_REG_DRAM_UPD_VALID << DRAM_CLK_REG_DRAM_UPD_OFFSET);
	writel(reg_val, SUNXI_CCU_APP_BASE + DRAM_CLK_REG);
	udelay(5);

	/* Enable MCTL (Memory Controller) clock */
	writel(0x8000, (MCTL_PHY_BASE + MCTL_PHY_CLKEN));
	udelay(10);
}

/**
 * @brief Initializes the memory controller with the specified parameters.
 * 
 * This function configures the memory controller by setting various parameters such as SDRAM type, 
 * word width, rank, bank, row, and address mapping. It also configures additional settings like the 
 * controller wait time, ODTMAP, and specific register values for different DRAM configurations.
 *
 * @param para Pointer to a structure containing the DRAM parameters. The structure should contain
 *             values such as DRAM type, size, ranks, timing parameters, and other relevant settings.
 * 
 * @note This function assumes that the base addresses for the memory controller (MCTL_COM_BASE, 
 *       MCTL_PHY_BASE) and the relevant register offsets are correctly defined elsewhere in the code.
 *       The specific register settings depend on the DRAM type and configuration (e.g., LPDDR2, LPDDR3).
 *
 * @return void
 */
static void mctl_com_init(dram_para_t *para) {
	uint32_t val, width;
	unsigned long ptr;
	int i;

	// Setting controller wait time
	clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_DBGCR), 0x3f00, 0x2000);

	// Set SDRAM type and word width
	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)) & ~0x00fff000;
	val |= (para->dram_type & 0x7) << 16;  ///< DRAM type
	val |= (~para->dram_para2 & 0x1) << 12;///< DQ width
	val |= (1 << 22);					   ///< Additional configuration flag
	if (para->dram_type == SUNXI_DRAM_TYPE_LPDDR2 || para->dram_type == SUNXI_DRAM_TYPE_LPDDR3) {
		val |= (1 << 19);// Type 6 and 7 must use 1T
	} else {
		if (para->dram_tpr13 & (1 << 5))
			val |= (1 << 19);
	}
	writel(val, (MCTL_COM_BASE + MCTL_COM_WORK_MODE0));

	// Initialize rank, bank, row for single/dual or two different ranks
	if ((para->dram_para2 & (1 << 8)) && ((para->dram_para2 & 0xf000) != 0x1000))
		width = 32;// 32-bit width if dual-rank is configured
	else
		width = 16;// 16-bit width otherwise

	ptr = (MCTL_COM_BASE + MCTL_COM_WORK_MODE0);
	for (i = 0; i < width; i += 16) {
		val = readl(ptr) & 0xfffff000;

		val |= (para->dram_para2 >> 12) & 0x3;					 ///< Rank
		val |= ((para->dram_para1 >> (i + 12)) << 2) & 0x4;		 ///< Bank - 2
		val |= (((para->dram_para1 >> (i + 4)) - 1) << 4) & 0xff;///< Row - 1

		// Convert from page size to column address width - 3
		switch ((para->dram_para1 >> i) & 0xf) {
			case 8:
				val |= 0xa00;
				break;
			case 4:
				val |= 0x900;
				break;
			case 2:
				val |= 0x800;
				break;
			case 1:
				val |= 0x700;
				break;
			default:
				val |= 0x600;
				break;
		}
		writel(val, ptr);
		ptr += 4;
	}

	// Set ODTMAP based on the number of ranks in use
	val = (readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)) & 0x1) ? 0x303 : 0x201;
	writel(val, (MCTL_PHY_BASE + MCTL_PHY_ODTMAP));

	// Set mctl reg 3c4 to zero when using half DQ
	if (para->dram_para2 & (1 << 0))
		writel(0, (MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(1)));

	// Set DRAM address mapping from dram_tpr4 parameter
	if (para->dram_tpr4) {
		setbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE0), (para->dram_tpr4 & 0x3) << 25);
		setbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE1), (para->dram_tpr4 & 0x7fc) << 10);
	}
}

/**
 * @brief Initializes the memory controller channel with the provided DRAM parameters.
 * 
 * This function configures the memory controller channel based on the provided DRAM parameters. 
 * It sets the DDR clock, adjusts PHY registers for DQS gating mode, enables/disable ODT (On-Die Termination),
 * configures PLL and SSCG, applies timing and voltage parameters, and performs necessary DRAM controller 
 * initialization and calibration steps. The function also handles power gating and waits for the status 
 * signals to complete.
 * 
 * @param ch_index The index of the memory channel to initialize (not used in this implementation).
 * @param para Pointer to a structure containing the DRAM configuration parameters. This structure includes:
 *             - dram_clk: DRAM clock frequency
 *             - dram_odt_en: ODT enable flag
 *             - dram_type: DRAM type (e.g., DDR3, LPDDR2, LPDDR3)
 *             - dram_tpr13: Timing parameter for DQS gating
 *             - dram_zq: ZQ calibration parameter
 *             - dram_para2: Additional parameters, including rank configuration
 *             - dram_tpr4: Additional timing parameter
 *             - dram_tpr13: Timing parameter for DQS gating mode
 * 
 * @return 1 if initialization was successful, 0 if there was a ZQ calibration error.
 * 
 * @note The function waits for several internal signals and completes multiple calibration steps. It assumes 
 *       the DRAM controller's base address (MCTL_PHY_BASE, MCTL_COM_BASE) and the relevant register offsets 
 *       are correctly defined elsewhere in the system.
 * 
 * @warning This function relies on hardware-specific timing parameters and assumes correct memory controller 
 *          configuration for the target platform. Modifications to the registers or incorrect parameter values 
 *          may result in improper initialization or system instability.
 */
static unsigned int mctl_channel_init(unsigned int ch_index, dram_para_t *para) {
	unsigned int val, dqs_gating_mode;

	dqs_gating_mode = (para->dram_tpr13 & 0xc) >> 2;

	// set DDR clock to half of CPU clock
	clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_TMR), 0xfff, (para->dram_clk / 2) - 1);

	// MRCTRL0 nibble 3 undocumented
	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xf00, 0x300);

	if (para->dram_odt_en)
		val = 0;
	else
		val = (1 << 5);

	// DX0GCR0
	if (para->dram_clk > 672)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(0)), 0xf63e, val);
	else
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(0)), 0xf03e, val);

	// DX1GCR0
	if (para->dram_clk > 672) {
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(0)), 0x400);
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(1)), 0xf63e, val);
	} else {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(1)), 0xf03e, val);
	}

	// (MCTL_PHY_BASE+MCTL_PHY_ACIOCR0) undocumented
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR0), (1 << 1));

	eye_delay_compensation(para);

	// set PLL SSCG
	val = readl((MCTL_PHY_BASE + MCTL_PHY_PGCR2));
	if (dqs_gating_mode == 1) {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0, 0);
		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_DQSGMR), 0x107);
	} else if (dqs_gating_mode == 2) {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0, 0x80);

		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DQSGMR), 0x107, (((para->dram_tpr13 >> 16) & 0x1f) - 2) | 0x100);
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), (1 << 31), (1 << 27));
	} else {
		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0x40);
		udelay(10);
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0);
	}

	if (para->dram_type == SUNXI_DRAM_TYPE_LPDDR2 || para->dram_type == SUNXI_DRAM_TYPE_LPDDR3) {
		if (dqs_gating_mode == 1)
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0x080000c0, 0x80000000);
		else
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0x77000000, 0x22000000);
	}

	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DTCR), 0x0fffffff, (para->dram_para2 & (1 << 12)) ? 0x03000001 /* 2 rank */ : 0x01003087 /* 1 rank */);

	if (readl(SUNXI_R_CPUCFG_SUP_STAN_FLAG) & (1 << 16)) {
		clrbits_le32(VDD_SYS_PWROFF_GATING_REG, 0x2);
		udelay(10);
	}

	// Set ZQ config
	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_ZQCR), 0x3ffffff, (para->dram_zq & 0x00ffffff) | (1 << 25));

	// Initialise DRAM controller
	if (dqs_gating_mode == 1) {
		// writel(0x52, (MCTL_PHY_BASE+MCTL_PHY_PIR)); // prep PHY reset + PLL init
		// + z-cal
		writel(0x53, (MCTL_PHY_BASE + MCTL_PHY_PIR));// Go

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}// wait for IDONE
		udelay(10);

		// 0x520 = prep DQS gating + DRAM init + d-cal
		if (para->dram_type == SUNXI_DRAM_TYPE_DDR3)
			writel(0x5a0, (MCTL_PHY_BASE + MCTL_PHY_PIR));// + DRAM reset
		else
			writel(0x520, (MCTL_PHY_BASE + MCTL_PHY_PIR));
	} else {
		if ((readl(SUNXI_R_CPUCFG_SUP_STAN_FLAG) & (1 << 16)) == 0) {
			// prep DRAM init + PHY reset + d-cal + PLL init + z-cal
			if (para->dram_type == SUNXI_DRAM_TYPE_DDR3)
				writel(0x1f2, (MCTL_PHY_BASE + MCTL_PHY_PIR));// + DRAM reset
			else
				writel(0x172, (MCTL_PHY_BASE + MCTL_PHY_PIR));
		} else {
			// prep PHY reset + d-cal + z-cal
			writel(0x62, (MCTL_PHY_BASE + MCTL_PHY_PIR));
		}
	}

	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PIR), 0x1);// GO

	udelay(10);
	while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}// wait for IDONE

	if (readl(SUNXI_R_CPUCFG_SUP_STAN_FLAG) & (1 << 16)) {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000, 0x04000000);
		udelay(10);

		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), 0x1);

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x7) != 0x3) {}

		clrbits_le32(VDD_SYS_PWROFF_GATING_REG, 0x1);
		udelay(10);

		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), 0x1);

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x7) != 0x1) {}

		udelay(15);

		if (dqs_gating_mode == 1) {
			clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0);
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000, 0x02000000);
			udelay(1);
			writel(0x401, (MCTL_PHY_BASE + MCTL_PHY_PIR));

			while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}
		}
	}

	// Check for training error
	if (readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & (1 << 20)) {
		printk_error("ZQ calibration error, check external 240 ohm resistor\n");
		return 0;
	}

	// STATR = Zynq STAT? Wait for status 'normal'?
	while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x1) == 0) {}

	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_RFSHCTL0), (1 << 31));
	udelay(10);
	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_RFSHCTL0), (1 << 31));
	udelay(10);
	setbits_le32((MCTL_COM_BASE + MCTL_COM_CCCR), (1 << 31));
	udelay(10);

	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000);

	if (dqs_gating_mode == 1)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0xc0, 0x40);

	return 1;
}

/**
 * @brief Calculates the size of a memory rank based on the provided register value.
 * 
 * This function computes the size of a memory rank using the information embedded in 
 * the given register value. The register value is interpreted as follows:
 * - Bits 8-11 (page size): Represents the page size offset.
 * - Bits 4-7 (row width): Represents the row width offset.
 * - Bits 2-3 (bank count): Represents the bank count offset.
 * The function combines these values to calculate the rank size in terms of memory blocks.
 * 
 * The calculation subtracts 14 (representing 1MB = 20 bits, minus an additional 6 bits),
 * and then returns the size of the memory rank as a power of 2.
 * 
 * @param regval The register value containing memory configuration information. The value is assumed to 
 *               be formatted with page size, row width, and bank count information located in specific bit ranges:
 *               - Bits 8-11 (page size)
 *               - Bits 4-7 (row width)
 *               - Bits 2-3 (bank count)
 * 
 * @return The calculated size of the memory rank in bytes as a power of 2 (i.e., 2^bits).
 * 
 * @note The function assumes that the provided register value corresponds to valid memory configuration data 
 *       (i.e., it is already correctly formatted according to the memory controller specifications).
 */
static unsigned int calculate_rank_size(uint32_t regval) {
	unsigned int bits;

	bits = (regval >> 8) & 0xf;	 /* Page size - 3 */
	bits += (regval >> 4) & 0xf; /* Row width - 1 */
	bits += (regval >> 2) & 0x3; /* Bank count - 2 */
	bits -= 14;					 /* 1MB = 20 bits, minus above 6 = 14 */

	return 1U << bits;
}

/**
 * @brief Retrieves the total size of the DRAM.
 * 
 * This function calculates the total size of the DRAM by reading memory configuration 
 * values from two registers (`MCTL_COM_WORK_MODE0` and `MCTL_COM_WORK_MODE1`) and interpreting 
 * the memory rank configurations. It determines whether the system has a single rank, two identical ranks, 
 * or two distinct ranks based on the values read from the registers.
 * 
 * - If the system has a single rank, the size of that rank is returned.
 * - If the system has two identical ranks, the size of one rank is multiplied by two.
 * - If the system has two distinct ranks, the sizes of both ranks are added together.
 * 
 * The function uses the `calculate_rank_size()` helper function to determine the size of individual ranks.
 * 
 * @return The total size of the DRAM in bytes.
 * 
 * @note The function assumes the DRAM configuration is valid and the base addresses 
 *       (`MCTL_COM_BASE + MCTL_COM_WORK_MODE0`, `MCTL_COM_BASE + MCTL_COM_WORK_MODE1`) are correctly 
 *       mapped and accessible in the memory space.
 */
static unsigned int get_dram_size(void) {
	uint32_t val;
	unsigned int size;

	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)); /* MCTL_COM_WORK_MODE0 */
	size = calculate_rank_size(val);

	if ((val & 0x3) == 0) /* Single rank */
		return size;

	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE1)); /* MCTL_WORK_MODE1 */
	if ((val & 0x3) == 0)								/* Two identical ranks */
		return size * 2;

	/* Add sizes of both ranks */
	return size + calculate_rank_size(val);
}

/**
 * @brief Detects the DQS gate state and updates DRAM parameters based on the detected configuration.
 * 
 * This function performs a series of hardware register reads to detect the state of the DQS (Data Strobe) gate 
 * in a DRAM system. It checks the configuration of the DRAM system and modifies the `dram_para2` field of the 
 * `dram_para_t` structure to reflect the detected DRAM parameters, such as rank and DQ (Data Queue) configuration. 
 * Based on the detection, the function will set the appropriate flags for dual rank, single rank, full DQ, or 
 * half DQ memory configurations.
 * 
 * The function reads values from specific registers:
 * - `MCTL_PHY_PGSR0` to check if the DRAM is dual rank.
 * - `MCTL_PHY_DXnGSR0(0)` and `MCTL_PHY_DXnGSR0(1)` to check the state of the DQS gate and determine the DQ configuration.
 * 
 * The detected configuration is then used to update `para->dram_para2`, and appropriate debug messages are logged.
 * 
 * @param para Pointer to the `dram_para_t` structure which holds DRAM configuration parameters.
 * 
 * @return 1 if the DRAM configuration was successfully detected and updated, 0 if no valid configuration was detected.
 * 
 * @note This function assumes that the register addresses (`MCTL_PHY_BASE`, `MCTL_PHY_PGSR0`, etc.) are correctly 
 *       mapped and accessible in the memory space. It also relies on the presence of specific debug logging 
 *       mechanisms (`printk_debug`).
 */
static int dqs_gate_detect(dram_para_t *para) {
	uint32_t dx0 = 0, dx1 = 0;

	if ((readl(MCTL_PHY_BASE + MCTL_PHY_PGSR0) & BIT(22)) == 0) {
		para->dram_para2 = (para->dram_para2 & ~0xf) | BIT(12);
		printk_debug("dual rank and full DQ\n");

		return 1;
	}

	dx0 = (readl(MCTL_PHY_BASE + MCTL_PHY_DXnGSR0(0)) & 0x3000000) >> 24;
	if (dx0 == 0) {
		para->dram_para2 = (para->dram_para2 & ~0xf) | 0x1001;
		printk_debug("dual rank and half DQ\n");

		return 1;
	}

	if (dx0 == 2) {
		dx1 = (readl(MCTL_PHY_BASE + MCTL_PHY_DXnGSR0(1)) & 0x3000000) >> 24;
		if (dx1 == 2) {
			para->dram_para2 = para->dram_para2 & ~0xf00f;
			printk_debug("single rank and full DQ\n");
		} else {
			para->dram_para2 = (para->dram_para2 & ~0xf00f) | BIT(0);
			printk_debug("single rank and half DQ\n");
		}

		return 1;
	}

	printk_debug("DQS GATE DX0 state: %lu\n", dx0);
	printk_debug("DQS GATE DX1 state: %lu\n", dx1);

	if ((para->dram_tpr13 & BIT(29)) == 0) {
		return 0;
	}

	return 0;
}

/**
 * @brief Performs a simple write-read test on the DRAM to verify its functionality.
 * 
 * This function writes two distinct patterns to different memory regions in the DRAM and then reads back the
 * written values to check for consistency. The test is performed on half of the specified memory size, with
 * pattern `patt1` written to the first half and pattern `patt2` written to the second half. The function verifies
 * that the read values match the expected patterns at the corresponding addresses. If any mismatch is found, the
 * function logs an error and returns a failure code.
 * 
 * The function works as follows:
 * 1. It calculates the offset as half of the total memory size (`mem_mb`), and uses this offset to write to the
 *    second half of the memory.
 * 2. It writes `patt1` and `patt2` to the first and second halves of the memory, respectively.
 * 3. It reads the values from both halves of the memory and compares them to the expected patterns (`patt1` and `patt2`).
 * 4. If any read value does not match the expected value, an error is logged and the function returns `1` indicating failure.
 * 5. If all values match, the function prints a success message and returns `0` indicating success.
 * 
 * @param mem_mb Total size of the DRAM in megabytes.
 * @param len Number of memory locations (in words) to test.
 * 
 * @return 0 if the test passes (all values match the expected patterns), 1 if any mismatch is found.
 * 
 * @note The function assumes that the base address of the DRAM is specified by `CONFIG_SYS_SDRAM_BASE` and that the
 *       system supports 32-bit word writes and reads. The function also assumes that the memory is properly initialized.
 */
static int dramc_simple_wr_test(unsigned int mem_mb, int len) {
	unsigned int offs = (mem_mb / 2) << 18;// Half of memory size
	unsigned int patt1 = 0x01234567;
	unsigned int patt2 = 0xfedcba98;
	unsigned int *addr, v1, v2, i;

	// Write pattern1 to the first half of the memory
	addr = (unsigned int *) CONFIG_SYS_SDRAM_BASE;
	for (i = 0; i != len; i++, addr++) {
		writel(patt1 + i, (unsigned long) addr);
		writel(patt2 + i, (unsigned long) (addr + offs));
	}

	// Read back and check pattern1 for the first half and pattern2 for the second half
	addr = (unsigned int *) CONFIG_SYS_SDRAM_BASE;
	for (i = 0; i != len; i++) {
		v1 = readl((unsigned long) (addr + i));
		v2 = patt1 + i;
		if (v1 != v2) {
			printk_error("DRAM: simple test FAIL\n");
			printk_error("%x != %x at address %p\n", v1, v2, addr + i);
			return 1;
		}
		v1 = readl((unsigned long) (addr + offs + i));
		v2 = patt2 + i;
		if (v1 != v2) {
			printk_error("DRAM: simple test FAIL\n");
			printk_error("%x != %x at address %p\n", v1, v2, addr + offs + i);
			return 1;
		}
	}

	printk_debug("DRAM: simple test OK\n");
	return 0;
}

/**
 * @brief Initializes the Vref mode for the memory controller.
 * 
 * This function configures the voltage reference (Vref) settings for the memory controller by modifying the
 * appropriate registers. It uses the `dram_para_t` structure to determine the settings based on the provided
 * configuration parameters.
 * 
 * The function performs the following steps:
 * 1. Checks if a certain bit in `dram_tpr13` is set (bit 17). If it is, the function returns early and does
 *    nothing.
 * 2. Modifies the `MCTL_PHY_IOVCR0` register to set the appropriate I/O voltage reference values based on the
 *    `dram_tpr5` configuration. Only the lower 32 bits of `dram_tpr5` are used for this purpose.
 * 3. If bit 16 in `dram_tpr13` is not set, it modifies the `MCTL_PHY_IOVCR1` register to adjust another set of
 *    voltage reference settings based on the lower 7 bits of `dram_tpr6`.
 * 
 * The function utilizes the `clrsetbits_le32` helper to modify specific bits of the I/O voltage reference control
 * registers without affecting the other bits.
 * 
 * @param para Pointer to the `dram_para_t` structure containing the configuration parameters.
 * 
 * @note The function assumes that the memory controller base address (`MCTL_PHY_BASE`) and the corresponding
 *       register offsets (`MCTL_PHY_IOVCR0` and `MCTL_PHY_IOVCR1`) are defined elsewhere in the code. 
 *       Additionally, the function assumes that the `dram_para_t` structure contains valid values for the 
 *       voltage reference configuration.
 */
static void mctl_vrefzq_init(dram_para_t *para) {
	if (para->dram_tpr13 & (1 << 17))
		return;

	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_IOVCR0), 0x7f7f7f7f, para->dram_tpr5);

	// IOCVR1
	if ((para->dram_tpr13 & (1 << 16)) == 0)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_IOVCR1), 0x7f, para->dram_tpr6 & 0x7f);
}

/**
 * @brief Initializes the memory controller.
 * 
 * This function performs a three-stage initialization of the memory controller:
 * 1. **First stage**: Initializes the controller to establish the number of ranks and DQ width.
 * 2. **Second stage**: Initializes the controller to determine the actual RAM size.
 * 3. **Third stage**: Finalizes the initialization with the final settings.
 * 
 * The function performs the following operations in sequence:
 * 1. Calls `mctl_sys_init()` to initialize the system parameters.
 * 2. Calls `mctl_vrefzq_init()` to configure the voltage reference settings.
 * 3. Calls `mctl_com_init()` to initialize the memory controller communication settings.
 * 4. Calls `mctl_set_timing_params()` to set the timing parameters for the memory controller.
 * 5. Finally, calls `mctl_channel_init()` to initialize the memory channel with the given parameters.
 * 
 * @param para Pointer to the `dram_para_t` structure that holds the memory controller's configuration parameters.
 * 
 * @return int Returns the status of the memory channel initialization. A value of `0` typically indicates success.
 * 
 * @note The function assumes that the `mctl_sys_init()`, `mctl_vrefzq_init()`, `mctl_com_init()`, 
 *       `mctl_set_timing_params()`, and `mctl_channel_init()` functions are correctly implemented and
 *       that they are responsible for various aspects of memory controller configuration.
 */
static int mctl_core_init(dram_para_t *para) {
	mctl_sys_init(para);

	mctl_vrefzq_init(para);

	mctl_com_init(para);

	mctl_set_timing_params(para);

	return mctl_channel_init(0, para);
}

/**
 * @brief Scans and sizes a DRAM device by cycling through address lines to determine the configuration.
 * 
 * This function performs an auto-scan of the DRAM to determine its size by cycling through various address lines
 * and checking if they correspond to real memory addresses or mirrored addresses. The process involves adjusting
 * and testing column, row, and bank bit allocations, and determining the number of address lines for each part.
 * The results are then stored in the `dram_para1` and `dram_para2` parameters.
 * 
 * The procedure follows these steps:
 * 1. Initializes the DRAM controller using `mctl_core_init()`.
 * 2. Sets the rank count based on the DRAM configuration.
 * 3. For each rank, it:
 *    - Configures and tests the row address lines.
 *    - Tests the bank address lines.
 *    - Configures and tests the column address lines.
 * 4. Finally, checks if the configuration for rank 1 differs from rank 0 for dual rank configurations.
 * 
 * The results for row size, bank size, and page size are stored in the `dram_para1` structure, while the 
 * dual rank configuration is updated in `dram_para2`.
 * 
 * @param para Pointer to a `dram_para_t` structure that holds the configuration and result of the DRAM scan.
 * 
 * @return int Returns 1 on success, indicating the DRAM size scan was successful.
 * @retval 0 If the DRAM initialization failed.
 * 
 * @note The function assumes that the DRAM is mapped starting at `CONFIG_SYS_SDRAM_BASE`. It uses hardware-specific
 *       register operations to configure and test the DRAM controller.
 */
static int auto_scan_dram_size(dram_para_t *para) {
	uint32_t i = 0, j = 0, current_rank = 0;
	uint32_t rank_count = 1, addr_line = 0;
	uint32_t reg_val = 0, ret = 0, cnt = 0;
	volatile uint32_t mc_work_mode;
	uint32_t rank1_addr = CONFIG_SYS_SDRAM_BASE;

	// init core
	if (mctl_core_init(para) == 0) {
		printk_debug("DRAM initial error : 0!\n");
		return 0;
	}

	// Set rank_count to 2
	if ((((para->dram_para2 >> 12) & 0xf) == 0x1))
		rank_count = 2;

	for (current_rank = 0; current_rank < rank_count; current_rank++) {
		mc_work_mode = ((MCTL_COM_BASE + MCTL_COM_WORK_MODE0) + 4 * current_rank);

		/* Set 16 Row 4Bank 512BPage for Rank 1 */
		if (current_rank == 1) {
			clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE0), 0xf0c, 0x6f0);
			clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE1), 0xf0c, 0x6f0);
			/* update Rank 1 addr */
			rank1_addr = CONFIG_SYS_SDRAM_BASE + (0x1 << 27);
		}

		/* write test pattern */
		for (i = 0; i < 64; i++) { writel((i % 2) ? (CONFIG_SYS_SDRAM_BASE + 4 * i) : (~(CONFIG_SYS_SDRAM_BASE + 4 * i)), CONFIG_SYS_SDRAM_BASE + 4 * i); }
		/* flush cache */
		data_sync_barrier();

		/* set row mode */
		clrsetbits_le32(mc_work_mode, 0xf0c, 0x6f0);
		udelay(2);

		for (i = 11; i < 17; i++) {
			ret = CONFIG_SYS_SDRAM_BASE + (1 << (i + 2 + 9)); /* row-bank-column */
			cnt = 0;
			for (j = 0; j < 64; j++) {
				reg_val = (j % 2) ? (rank1_addr + 4 * j) : (~(rank1_addr + 4 * j));
				if (reg_val == readl(ret + j * 4)) {
					cnt++;
				} else
					break;
			}
			if (cnt == 64) {
				break;
			}
		}
		if (i >= 16)
			i = 16;
		addr_line += i;

		printk_debug("rank %lu row = %lu \n", current_rank, i);

		/* Store rows in para 1 */
		para->dram_para1 &= ~(0xffU << (16 * current_rank + 4));
		para->dram_para1 |= (i << (16 * current_rank + 4));

		/* Set bank mode for current rank */
		if (current_rank == 1) { /* Set bank mode for rank0 */
			clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE0), 0xffc, 0x6a4);
		}

		/* Set bank mode for current rank */
		clrsetbits_le32(mc_work_mode, 0xffc, 0x6a4);
		udelay(1);

		for (i = 0; i < 1; i++) {
			ret = CONFIG_SYS_SDRAM_BASE + (0x1U << (i + 2 + 9));
			cnt = 0;
			for (j = 0; j < 64; j++) {
				reg_val = (j % 2) ? (rank1_addr + 4 * j) : (~(rank1_addr + 4 * j));
				if (reg_val == readl(ret + j * 4)) {
					cnt++;
				} else
					break;
			}
			if (cnt == 64) {
				break;
			}
		}

		addr_line += i + 2;
		printk_debug("rank %lu bank = %lu \n", current_rank, (4 + i * 4));

		/* Store bank in para 1 */
		para->dram_para1 &= ~(0xfU << (16 * current_rank + 12));
		para->dram_para1 |= (i << (16 * current_rank + 12));

		/* Set page mode for rank0 */
		if (current_rank == 1) {
			clrsetbits_le32(mc_work_mode, 0xffc, 0xaa0);
		}

		/* Set page mode for current rank */
		clrsetbits_le32(mc_work_mode, 0xffc, 0xaa0);
		udelay(2);

		/* Scan per address line, until address wraps (i.e. see shadow) */
		for (i = 9; i <= 13; i++) {
			ret = CONFIG_SYS_SDRAM_BASE + (0x1U << i);// column 40000000+（9~13）
			cnt = 0;
			for (j = 0; j < 64; j++) {
				reg_val = (j % 2) ? (CONFIG_SYS_SDRAM_BASE + 4 * j) : (~(CONFIG_SYS_SDRAM_BASE + 4 * j));
				if (reg_val == readl(ret + j * 4)) {
					cnt++;
				} else {
					break;
				}
			}
			if (cnt == 64) {
				break;
			}
		}

		if (i >= 13) {
			i = 13;
		}

		/* add page size */
		addr_line += i;

		if (i == 9) {
			i = 0;
		} else {
			i = (0x1U << (i - 10));
		}

		printk_debug("rank %lu page size = %lu KB \n", current_rank, i);

		/* Store page in para 1 */
		para->dram_para1 &= ~(0xfU << (16 * current_rank));
		para->dram_para1 |= (i << (16 * current_rank));
	}

	/* check dual rank config */
	if (rank_count == 2) {
		para->dram_para2 &= 0xfffff0ff;
		if ((para->dram_para1 & 0xffff) == (para->dram_para1 >> 16)) {
			printk_debug("rank1 config same as rank0\n");
		} else {
			para->dram_para2 |= 0x1 << 8;
			printk_debug("rank1 config different from rank0\n");
		}
	}
	return 1;
}

/**
 * @brief Automatically scans and detects DRAM rank and DQ width.
 * 
 * This function configures the DRAM controller to test for the number of ranks (1 or 2)
 * and the DQ width (full or half). It sets up the parameters with `dqs_gating_mode` 
 * equal to 1 and enables two ranks. It then configures the controller and performs 
 * tests to detect the rank and width. The function also resets the parameters to their 
 * original values after the scan.
 * 
 * The steps performed by this function are as follows:
 * 1. Save the original values of `dram_tpr13` and `dram_para1` in local variables.
 * 2. Set up the DRAM parameters with a specific configuration (dqs_gating_mode = 1, two ranks).
 * 3. Enable DQS probe mode by modifying the `dram_tpr13` register.
 * 4. Call `mctl_core_init()` to initialize the memory controller with the new configuration.
 * 5. Check if a specific bit in the PHY status register (`PGSR0`) is set, indicating an error.
 * 6. Call `dqs_gate_detect()` to detect the DRAM DQS gating status.
 * 7. If the detection fails, return `0` to indicate the scan was unsuccessful.
 * 8. After the scan, restore the original values of `dram_tpr13` and `dram_para1`.
 * 9. Return `1` to indicate the scan was successful.
 * 
 * @param para Pointer to the `dram_para_t` structure that holds the DRAM parameters to be tested.
 * 
 * @return int Returns `1` if the rank and width detection was successful, or `0` if it failed.
 * 
 * @note This function assumes that `mctl_core_init()` and `dqs_gate_detect()` are properly implemented.
 *       It also assumes that the DRAM controller and PHY registers are accessible and can be modified as needed.
 */
static int auto_scan_dram_rank_width(dram_para_t *para) {
	unsigned int s1 = para->dram_tpr13;
	unsigned int s2 = para->dram_para1;

	para->dram_para1 = 0x00b000b0;
	para->dram_para2 = (para->dram_para2 & ~0xf) | (1 << 12);

	/* set DQS probe mode */
	para->dram_tpr13 = (para->dram_tpr13 & ~0x8) | (1 << 2) | (1 << 0);

	mctl_core_init(para);

	if (readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & (1 << 20))
		return 0;

	if (dqs_gate_detect(para) == 0)
		return 0;

	para->dram_tpr13 = s1;
	para->dram_para1 = s2;

	return 1;
}

/**
 * @brief Automatically scans and configures the SDRAM topology.
 * 
 * This function determines the SDRAM topology by performing a series of scans. 
 * First, it scans and establishes the number of ranks and the DQ width. Then, it 
 * scans the SDRAM address lines to determine the size of each rank. After the scan, 
 * it updates the `dram_tpr13` register to reflect that the SDRAM topology has been 
 * successfully detected. The updated value of `dram_tpr13` ensures that the auto-scan 
 * is not repeated upon a re-initialization.
 * 
 * The function follows these steps:
 * 1. If the `dram_tpr13` register does not indicate that the number of ranks and DQ width
 *    have been determined (bit 14 not set), it calls `auto_scan_dram_rank_width()` to perform 
 *    the scan. If the scan fails, it logs an error message and returns `0`.
 * 2. If the `dram_tpr13` register does not indicate that the size of each rank is known 
 *    (bit 0 not set), it calls `auto_scan_dram_size()` to determine the size of each rank. 
 *    If the scan fails, it logs an error message and returns `0`.
 * 3. If bit 15 of `dram_tpr13` is not set, it sets the appropriate bits in `dram_tpr13` 
 *    to indicate that the SDRAM topology is now known.
 * 
 * @param para Pointer to the `dram_para_t` structure that holds the SDRAM parameters.
 * 
 * @return int Returns `1` if the SDRAM configuration was successfully determined, 
 *             or `0` if an error occurred during the scan.
 * 
 * @note The function assumes that `auto_scan_dram_rank_width()` and `auto_scan_dram_size()` 
 *       are properly implemented and return `0` on failure and non-zero on success.
 * 
 * @warning The function logs errors if the rank, width, or size detection fails, and it 
 *          ensures that re-initialization will not repeat the auto-scan if the SDRAM 
 *          topology is already known.
 */
static int auto_scan_dram_config(dram_para_t *para) {
	if (((para->dram_tpr13 & BIT(14)) == 0) && (auto_scan_dram_rank_width(para) == 0)) {
		printk_error("ERROR: auto scan dram rank & width failed\n");
		return 0;
	}

	if (((para->dram_tpr13 & BIT(0)) == 0) && (auto_scan_dram_size(para) == 0)) {
		printk_error("ERROR: auto scan dram size failed\n");
		return 0;
	}

	if ((para->dram_tpr13 & BIT(15)) == 0)
		para->dram_tpr13 |= BIT(14) | BIT(13) | BIT(1) | BIT(0);

	return 1;
}

/**
 * @brief Initializes the DRAM controller and configures memory parameters.
 * 
 * This function initializes the DRAM controller and configures various settings 
 * based on the provided `dram_para_t` structure. It configures the DRAM clock, 
 * type, ZQ calibration, SDRAM size, hardware auto-refresh, ODT (On Die Termination) 
 * settings, and more. The function also ensures that the DRAM controller is properly 
 * initialized for the specified memory type (DDR2, DDR3, LPDDR2, LPDDR3, etc.).
 * 
 * The following steps are performed:
 * 1. Prints out the DRAM boot info, clock speed, and type.
 * 2. Checks and configures the ZQ calibration based on the DRAM parameters.
 * 3. If the DRAM auto-scan is not already performed, it invokes `auto_scan_dram_config` 
 *    to scan and configure the DRAM topology.
 * 4. Initializes the core controller using `mctl_core_init`.
 * 5. Retrieves and sets the DRAM size. The size can be forced by the `dram_para2` parameter.
 * 6. Configures hardware auto-refresh settings if specified.
 * 7. Adjusts settings based on the DRAM type (e.g., LPDDR3 ODT delay, HDR/DDR dynamic).
 * 8. Disables ZQ calibration and sets the appropriate configurations for VTF and PAD hold.
 * 9. Enables all DRAM masters and performs simple write tests if necessary.
 * 
 * @param type The type of DRAM to initialize (not used in the function body, but typically 
 *             used to specify DDR2, DDR3, LPDDR2, etc.).
 * @param para Pointer to a `dram_para_t` structure containing DRAM configuration parameters, 
 *             such as clock, type, ODT settings, and other controller parameters.
 * 
 * @return int The size of the initialized DRAM in megabytes (MB) if successful, 
 *             or 0 if initialization fails.
 * 
 * @note This function assumes that all registers are mapped to their correct memory locations.
 *       The ZQ calibration and other settings might depend on the specific hardware 
 *       configuration and DRAM type.
 * 
 * @warning The function assumes that the proper base addresses and control registers for 
 *          the DRAM controller are defined and accessible in the system.
 */
static int init_DRAM(int type, dram_para_t *para) {
	uint32_t rc, mem_size_mb;

	printk_debug("DRAM BOOT DRIVE INFO: %s\n", "V0.1");
	printk_debug("DRAM CLK = %d MHz\n", para->dram_clk);
	printk_debug("DRAM Type = %d (2:DDR2,3:DDR3)\n", para->dram_type);
	if ((para->dram_odt_en & 0x1) == 0)
		printk_debug("DRAMC read ODT off\n");
	else
		printk_debug("DRAMC ZQ value: 0x%x\n", para->dram_zq);

	/* Test ZQ status */
	if (para->dram_tpr13 & (1 << 16)) {
		printk_debug("DRAM only have internal ZQ\n");
		setbits_le32(ZQ_CAL_CTRL_REG, (1 << 8));
		writel(0, ZQ_RES_CTRL_REG);
		udelay(10);
	} else {
		writel(0x0, ANALOG_PWROFF_GATING_REG);
		clrbits_le32(ZQ_CAL_CTRL_REG, 0x3);
		udelay(10);
		clrsetbits_le32(ZQ_CAL_CTRL_REG, (BIT(8) | BIT(2)), BIT(1));
		udelay(10);
		setbits_le32(ZQ_CAL_CTRL_REG, BIT(0));
		udelay(20);
		printk_debug("ZQ value = 0x%08x\n", readl(ZQ_RES_STATUS_REG));
	}

	/* Set SDRAM controller auto config */
	if ((para->dram_tpr13 & (1 << 0)) == 0) {
		if (auto_scan_dram_config(para) == 0) {
			printk_error("auto_scan_dram_config() FAILED\n");
			return 0;
		}
	}

	/* report ODT */
	rc = para->dram_mr1;
	if ((rc & 0x44) == 0)
		printk_debug("DRAM ODT off\n");
	else
		printk_debug("DRAM ODT value: 0x%08x\n", rc);

	/* Init core, final run */
	if (mctl_core_init(para) == 0) {
		printk_debug("DRAM initialisation error: 1\n");
		return 0;
	}

	/* Get SDRAM size
     * You can set dram_para2 to force set the dram size
     */
	rc = para->dram_para2;
	if (rc & (1 << 31)) {
		rc = (rc >> 16) & ~(1 << 15);
	} else {
		rc = get_dram_size();
		printk_info("DRAM: size = %uMB\n", rc);
		para->dram_para2 = (para->dram_para2 & 0xffffU) | rc << 16;
	}
	mem_size_mb = rc;

	/* Enable hardware auto refresh */
	if (para->dram_tpr13 & (1 << 30)) {
		rc = para->dram_tpr8;
		if (rc == 0)
			rc = 0x10000200;
		writel(rc, (MCTL_PHY_BASE + MCTL_PHY_ASRTC));
		writel(0x40a, (MCTL_PHY_BASE + MCTL_PHY_ASRC));
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), (1 << 0));
		printk_debug("Enable Auto SR\n");
	} else {
		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_ASRTC), 0xffff);
		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), 0x1);
	}

	/* Set HDR/DDR dynamic */
	if (para->dram_tpr13 & (1 << 9)) {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), 0xf000, 0x5000);
	} else {
		if (para->dram_type != SUNXI_DRAM_TYPE_LPDDR2)
			clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), 0xf000);
	}

	/* Disable ZQ calibration */
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ZQCR), (1 << 31));

	/* Set VTF feature */
	if (para->dram_tpr13 & (1 << 8))
		writel(readl((MCTL_PHY_BASE + MCTL_PHY_VTFCR)) | 0x300, (MCTL_PHY_BASE + MCTL_PHY_VTFCR));

	/* Set PAD Hold */
	if (para->dram_tpr13 & (1 << 16))
		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), (1 << 13));
	else
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), (1 << 13));

	/* Set LPDDR3 ODT delay */
	if (para->dram_type == SUNXI_DRAM_TYPE_LPDDR3)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_ODTCFG), 0xf0000, 0x1000);

	dram_enable_all_master();
	if (para->dram_tpr13 & (1 << 28)) {
		if ((readl(SUNXI_R_CPUCFG_SUP_STAN_FLAG) & (1 << 16)) || dramc_simple_wr_test(mem_size_mb, 4096))
			return 0;
	}

	return mem_size_mb;
}

/**
 * @brief Initializes the DRAM with the given parameters.
 * 
 * This function initializes the DRAM using the provided parameters encapsulated
 * in the `dram_para_t` structure. It calls the `init_DRAM` function to perform 
 * the actual initialization and returns the result.
 * 
 * @param para A pointer to the `dram_para_t` structure that contains DRAM 
 *             initialization parameters. The function casts this void pointer 
 *             to a `dram_para_t` pointer.
 * 
 * @return uint32_t Returns the result of the `init_DRAM` function call, which
 *                  represents the size of the initialized DRAM in MB. A return 
 *                  value of 0 indicates failure.
 */
uint32_t sunxi_dram_init(void *para) {
	dram_para_t *dram_para = (dram_para_t *) para;
	return init_DRAM(0, dram_para);
};
