/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dram.h>

#include <common.h>

#define DIV_ROUND_UP(a, b) (((a) + (b) -1) / (b))

#define CONFIG_SYS_SDRAM_BASE SDRAM_BASE

#ifndef SUNXI_SID_BASE
#define SUNXI_SID_BASE 0x3006200
#endif

static int ns_to_t(dram_para_t *para, int nanoseconds) {
	const uint32_t ctrl_freq = para->dram_clk / 2;

	return DIV_ROUND_UP(ctrl_freq * nanoseconds, 1000);
}

static void dram_enable_all_master(void) {
	writel(~0, (MCTL_COM_BASE + MCTL_COM_MAER0));
	writel(0xff, (MCTL_COM_BASE + MCTL_COM_MAER1));
	writel(0xffff, (MCTL_COM_BASE + MCTL_COM_MAER2));
	sdelay(10);
}

static void dram_disable_all_master(void) {
	writel(1, (MCTL_COM_BASE + MCTL_COM_MAER0));
	writel(0, (MCTL_COM_BASE + MCTL_COM_MAER1));
	writel(0, (MCTL_COM_BASE + MCTL_COM_MAER2));
	sdelay(10);
}

static void eye_delay_compensation(dram_para_t *para)// s1
{
	uint32_t delay, i = 0;

	// DATn0IOCR, n =  0...7
	delay = (para->dram_tpr11 & 0xf) << 9;
	delay |= (para->dram_tpr12 & 0xf) << 1;

	for (i = 0; i < 9; i++) setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(i)), delay);

	// DATn1IOCR, n =  0...7
	delay = (para->dram_tpr11 & 0xf0) << 5;
	delay |= (para->dram_tpr12 & 0xf0) >> 3;
	for (i = 0; i < 9; i++) setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(i)), delay);

	// PGCR0: assert AC loopback FIFO reset
	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), 0x04000000);

	// DQS0 read and write delay
	delay = (para->dram_tpr11 & 0xf0000) >> 7;
	delay |= (para->dram_tpr12 & 0xf0000) >> 15;
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(9)), delay); // DQS0 P
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX0IOCR(10)), delay);// DQS0 N

	// DQS1 read and write delay
	delay = (para->dram_tpr11 & 0xf00000) >> 11;
	delay |= (para->dram_tpr12 & 0xf00000) >> 19;
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(9)), delay); // DQS1 P
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DATX1IOCR(10)), delay);// DQS1 N

	// DQS0 enable bit delay
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnSDLR6(0)), (para->dram_tpr11 & 0xf0000) << 9);

	// DQS1 enable bit delay
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXnSDLR6(1)), (para->dram_tpr11 & 0xf00000) << 5);

	// PGCR0: release AC loopback FIFO reset
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR0), (1 << 26));

	sdelay(1);

	delay = (para->dram_tpr10 & 0xf0) << 4;

	// Set RAS CAS and CA delay
	for (i = 6; i < 27; ++i) setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(i)), delay);

	// Set CK CS delay
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(2)), (para->dram_tpr10 & 0x0f) << 8);
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(3)), (para->dram_tpr10 & 0x0f) << 8);
	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_ACIOCR1(28)), (para->dram_tpr10 & 0xf00) >> 4);
}

/*
 * Main purpose of the auto_set_timing routine seems to be to calculate all
 * timing settings for the specific type of sdram used. Read together with
 * an sdram datasheet for context on the various variables.
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
										//	    twr		= max(ns_to_t(para, 15), 2);
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
								   //	    trd2wr		= tcwl + 2 + twr;		// WL+BL/2+tWR
			twr2rd = tcwl + twtr;  // WL+tWTR

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
	/* TODO: dram_odt_en is either 0x0 or 0x1, so right shift looks weird */
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

// Purpose of this routine seems to be to initialize the PLL driving
// the MBUS and sdram.
//
static int ccu_set_pll_ddr_clk(int index, dram_para_t *para) {
	uint32_t val, clk, n;

	if (para->dram_tpr13 & (1 << 6))
		clk = para->dram_tpr9;
	else
		clk = para->dram_clk;

	// set VCO clock divider
	n = (clk * 2) / 24;

	val = readl((CCU_BASE + CCU_PLL_DDR_CTRL_REG));
	val &= 0xfff800fc;	// clear dividers
	val |= (n - 1) << 8;// set PLL division
	val |= 0xc0000000;	// enable PLL and LDO
	val &= 0xdfffffff;
	writel(val | 0x20000000, (CCU_BASE + CCU_PLL_DDR_CTRL_REG));

	// wait for PLL to lock
	while ((readl((CCU_BASE + CCU_PLL_DDR_CTRL_REG)) & 0x10000000) == 0) { ; }

	sdelay(20);

	// enable PLL output
	val = readl(CCU_BASE);
	val |= 0x08000000;
	writel(val, CCU_BASE);

	// turn clock gate on
	val = readl((CCU_BASE + CCU_DRAM_CLK_REG));
	val &= 0xfcfffce0;// select DDR clk source, n=1, m=1
	val |= 0x80000000;// turn clock on
	writel(val, (CCU_BASE + CCU_DRAM_CLK_REG));

	return n * 24;
}

// Main purpose of sys_init seems to be to initalise the clocks for
// the sdram controller.
//
static void mctl_sys_init(dram_para_t *para) {
	// assert MBUS reset
	clrbits_le32((CCU_BASE + CCU_MBUS_CLK_REG), (1 << 30));

	// turn off sdram clock gate, assert sdram reset
	clrbits_le32((CCU_BASE + CCU_DRAM_BGR_REG), 0x10001);
	clrsetbits_le32((CCU_BASE + CCU_DRAM_CLK_REG), (1 << 31), (1 << 27));
	sdelay(10);

	// set ddr pll clock
	para->dram_clk = ccu_set_pll_ddr_clk(0, para) / 2;
	sdelay(100);
	dram_disable_all_master();

	// release sdram reset
	setbits_le32((CCU_BASE + CCU_DRAM_BGR_REG), (1 << 16));

	// release MBUS reset
	setbits_le32((CCU_BASE + CCU_MBUS_CLK_REG), (1 << 30));
	setbits_le32((CCU_BASE + CCU_DRAM_CLK_REG), (1 << 30));

	sdelay(5);

	// turn on sdram clock gate
	setbits_le32((CCU_BASE + CCU_DRAM_BGR_REG), (1 << 0));

	// turn dram clock gate on, trigger sdr clock update
	setbits_le32((CCU_BASE + CCU_DRAM_CLK_REG), (1 << 31) | (1 << 27));
	sdelay(5);

	// mCTL clock enable
	writel(0x8000, (MCTL_PHY_BASE + MCTL_PHY_CLKEN));
	sdelay(10);
}

// The main purpose of this routine seems to be to copy an address configuration
// from the dram_para1 and dram_para2 fields to the PHY configuration registers
// (MCTL_COM_WORK_MODE0, MCTL_COM_WORK_MODE1).
//
static void mctl_com_init(dram_para_t *para) {
	uint32_t val, width;
	unsigned long ptr;
	int i;

	// Setting controller wait time
	clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_DBGCR), 0x3f00, 0x2000);

	// set SDRAM type and word width
	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)) & ~0x00fff000;
	val |= (para->dram_type & 0x7) << 16;  // DRAM type
	val |= (~para->dram_para2 & 0x1) << 12;// DQ width
	val |= (1 << 22);					   // ??
	if (para->dram_type == SUNXI_DRAM_TYPE_LPDDR2 || para->dram_type == SUNXI_DRAM_TYPE_LPDDR3) {
		val |= (1 << 19);// type 6 and 7 must use 1T
	} else {
		if (para->dram_tpr13 & (1 << 5))
			val |= (1 << 19);
	}
	writel(val, (MCTL_COM_BASE + MCTL_COM_WORK_MODE0));

	// init rank / bank / row for single/dual or two different ranks
	if ((para->dram_para2 & (1 << 8)) && ((para->dram_para2 & 0xf000) != 0x1000))
		width = 32;
	else
		width = 16;

	ptr = (MCTL_COM_BASE + MCTL_COM_WORK_MODE0);
	for (i = 0; i < width; i += 16) {
		val = readl(ptr) & 0xfffff000;

		val |= (para->dram_para2 >> 12) & 0x3;					 // rank
		val |= ((para->dram_para1 >> (i + 12)) << 2) & 0x4;		 // bank - 2
		val |= (((para->dram_para1 >> (i + 4)) - 1) << 4) & 0xff;// row - 1

		// convert from page size to column addr width - 3
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

	// set ODTMAP based on number of ranks in use
	val = (readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)) & 0x1) ? 0x303 : 0x201;
	writel(val, (MCTL_PHY_BASE + MCTL_PHY_ODTMAP));

	// set mctl reg 3c4 to zero when using half DQ
	if (para->dram_para2 & (1 << 0))
		writel(0, (MCTL_PHY_BASE + MCTL_PHY_DXnGCR0(1)));

	// set dram address mapping from dram_tpr4 param
	if (para->dram_tpr4) {
		setbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE0), (para->dram_tpr4 & 0x3) << 25);
		setbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE1), (para->dram_tpr4 & 0x7fc) << 10);
	}
}

static const uint8_t ac_remapping_tables[][22] = {
		/* No Remap */
		[0] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		/* V853 DDR3 */
		[1] = {0x5, 0x13, 0x4, 0xF, 0xC, 0x3, 0xD, 0xE, 0x1, 0xA, 0x16, 0x14, 0x11, 0x10, 0x6, 0x8, 0x7, 0x9, 0x15, 0x2, 0xB, 0x12},
		/* V853s SIP Winbond 128M DDR3 */
		[2] = {0x13, 0x4, 0x12, 0x2, 0x6, 0x3, 0x5, 0x7, 0xC, 0x8, 0xD, 0xA, 0x9, 0x0, 0x0, 0x0, 0x16, 0x1, 0xB, 0x11, 0x15, 0x14},
};

/*
 * This routine chooses one of several remapping tables for 22 lines.
 * It is unclear which lines are being remapped. It seems to pick
 * table cfg7 for the Nezha board.
 */
static void mctl_phy_ac_remapping(dram_para_t *para) {
	const uint8_t *cfg;
	uint32_t fuse, val;

	/*
   * Only DDR2 and DDR3 Support Remap
   */
	if (para->dram_type != SUNXI_DRAM_TYPE_DDR2 && para->dram_type != SUNXI_DRAM_TYPE_DDR3)
		return;

	fuse = (readl(SYS_SID_BASE + SYS_EFUSE_REG) & 0xf00) >> 8;
	printk_debug("DDR efuse: 0x%x\n", fuse);

	if (para->dram_type == SUNXI_DRAM_TYPE_DDR2) {
		return;
	} else if (fuse == 0x6) {
		cfg = ac_remapping_tables[0];
	} else {
		cfg = ac_remapping_tables[1];
	}

	val = (cfg[1] << 10) | (32 * cfg[0]) | 1 | (cfg[2] << 15) | (cfg[3] << 20) | (cfg[4] << 25);
	writel(val, (MCTL_COM_BASE + MCTL_COM_REMAP0));
	val = (cfg[7] << 10) | (32 * cfg[6]) | cfg[5] | (cfg[8] << 15) | (cfg[9] << 20) | (cfg[10] << 25);
	writel(val, (MCTL_COM_BASE + MCTL_COM_REMAP1));
	val = (cfg[13] << 10) | (32 * cfg[12]) | cfg[11] | (cfg[14] << 15) | (cfg[15] << 20);
	writel(val, (MCTL_COM_BASE + MCTL_COM_REMAP2));
	val = (cfg[18] << 10) | (32 * cfg[17]) | cfg[16] | (cfg[19] << 15) | (cfg[20] << 20) | (cfg[21] << 25);
	writel(val, (MCTL_COM_BASE + MCTL_COM_REMAP3));

	printk_trace("MCTL_COM_REMAP0 = 0x%x\n", readl((MCTL_COM_BASE + MCTL_COM_REMAP0)));
	printk_trace("MCTL_COM_REMAP1 = 0x%x\n", readl((MCTL_COM_BASE + MCTL_COM_REMAP1)));
	printk_trace("MCTL_COM_REMAP2 = 0x%x\n", readl((MCTL_COM_BASE + MCTL_COM_REMAP2)));
	printk_trace("MCTL_COM_REMAP3 = 0x%x\n", readl((MCTL_COM_BASE + MCTL_COM_REMAP3)));
}

// Init the controller channel. The key part is placing commands in the main
// command register (PIR, 0x3103000) and checking command status (PGSR0, 0x3103010).
//
static uint32_t mctl_channel_init(uint32_t ch_index, dram_para_t *para) {
	uint32_t val, dqs_gating_mode;

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

	// set PLL SSCG ?
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
		sdelay(10);
		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0);
	}

	if (para->dram_type == SUNXI_DRAM_TYPE_LPDDR2 || para->dram_type == SUNXI_DRAM_TYPE_LPDDR3) {
		if (dqs_gating_mode == 1)
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0x080000c0, 0x80000000);
		else
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0x77000000, 0x22000000);
	}

	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DTCR), 0x0fffffff, (para->dram_para2 & (1 << 12)) ? 0x03000001 : 0x01000007);

	if (readl((SUNXI_R_CPUCFG_BASE + SUNXI_R_CPUCFG_SUP_STAN_FLAG)) & (1 << 16)) {
		clrbits_le32((R_PRCM_BASE + VDD_SYS_PWROFF_GATING_REG), 0x2);
		sdelay(10);
	}

	// Set ZQ config
	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_ZQCR), 0x3ffffff, (para->dram_zq & 0x00ffffff) | (1 << 25));

	// Initialise DRAM controller
	if (dqs_gating_mode == 1) {
		// writel(0x52, (MCTL_PHY_BASE+MCTL_PHY_PIR)); // prep PHY reset + PLL init
		// + z-cal
		writel(0x53, (MCTL_PHY_BASE + MCTL_PHY_PIR));// Go

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}// wait for IDONE
		sdelay(10);

		// 0x520 = prep DQS gating + DRAM init + d-cal
		if (para->dram_type == SUNXI_DRAM_TYPE_DDR3)
			writel(0x5a0,
				   (MCTL_PHY_BASE + MCTL_PHY_PIR));// + DRAM reset
		else
			writel(0x520, (MCTL_PHY_BASE + MCTL_PHY_PIR));
	} else {
		if ((readl((SUNXI_R_CPUCFG_BASE + SUNXI_R_CPUCFG_SUP_STAN_FLAG)) & (1 << 16)) == 0) {
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

	sdelay(10);
	while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}// wait for IDONE

	if (readl((SUNXI_R_CPUCFG_BASE + SUNXI_R_CPUCFG_SUP_STAN_FLAG)) & (1 << 16)) {
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000, 0x04000000);
		sdelay(10);

		setbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), 0x1);

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x7) != 0x3) {}

		clrbits_le32((R_PRCM_BASE + VDD_SYS_PWROFF_GATING_REG), 0x1);
		sdelay(10);

		clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PWRCTL), 0x1);

		while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x7) != 0x1) {}

		sdelay(15);

		if (dqs_gating_mode == 1) {
			clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR2), 0xc0);
			clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000, 0x02000000);
			sdelay(1);
			writel(0x401, (MCTL_PHY_BASE + MCTL_PHY_PIR));

			while ((readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & 0x1) == 0) {}
		}
	}

	// Check for training error
	if (readl((MCTL_PHY_BASE + MCTL_PHY_PGSR0)) & (1 << 20)) {
		printk(LOG_LEVEL_ERROR, "ZQ calibration error, check external 240 ohm resistor\n");
		return 0;
	}

	// STATR = Zynq STAT? Wait for status 'normal'?
	while ((readl((MCTL_PHY_BASE + MCTL_PHY_STATR)) & 0x1) == 0) {}

	setbits_le32((MCTL_PHY_BASE + MCTL_PHY_RFSHCTL0), (1 << 31));
	sdelay(10);
	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_RFSHCTL0), (1 << 31));
	sdelay(10);
	setbits_le32((MCTL_COM_BASE + MCTL_COM_CCCR), (1 << 31));
	sdelay(10);

	clrbits_le32((MCTL_PHY_BASE + MCTL_PHY_PGCR3), 0x06000000);

	if (dqs_gating_mode == 1)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_DXCCR), 0xc0, 0x40);

	return 1;
}

static uint32_t calculate_rank_size(uint32_t regval) {
	uint32_t bits;

	bits = (regval >> 8) & 0xf;	 /* page size - 3 */
	bits += (regval >> 4) & 0xf; /* row width - 1 */
	bits += (regval >> 2) & 0x3; /* bank count - 2 */
	bits -= 14;					 /* 1MB = 20 bits, minus above 6 = 14 */

	return 1U << bits;
}

/*
 * The below routine reads the dram config registers and extracts
 * the number of address bits in each rank available. It then calculates
 * total memory size in MB.
 */
static uint32_t dramc_get_dram_size(void) {
	uint32_t val;
	uint32_t size;

	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE0)); /* MCTL_COM_WORK_MODE0 */
	size = calculate_rank_size(val);

	if ((val & 0x3) == 0) /* single rank? */
		return size;

	val = readl((MCTL_COM_BASE + MCTL_COM_WORK_MODE1)); /* MCTL_WORK_MODE1 */
	if ((val & 0x3) == 0)								/* two identical ranks? */
		return size * 2;

	/* add sizes of both ranks */
	return size + calculate_rank_size(val);
}

/*
 * The below routine reads the command status register to extract
 * DQ width and rank count. This follows the DQS training command in
 * channel_init. If error bit 22 is reset, we have two ranks and full DQ.
 * If there was an error, figure out whether it was half DQ, single rank,
 * or both. Set bit 12 and 0 in dram_para2 with the results.
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

	if ((para->dram_tpr13 & BIT(29)) == 0)
		return 0;

	printk_debug("DX0 state: %d\n", dx0);
	printk_debug("DX1 state: %d\n", dx1);

	return 0;
}

static int dramc_simple_wr_test(uint32_t mem_mb, int len) {
	uint32_t offs = (mem_mb / 2) << 18;// half of memory size
	uint32_t patt1 = 0x01234567;
	uint32_t patt2 = 0xfedcba98;
	uint32_t *addr, v1, v2, i;

	addr = (uint32_t *) CONFIG_SYS_SDRAM_BASE;
	for (i = 0; i != len; i++, addr++) {
		writel(patt1 + i, (unsigned long) addr);
		writel(patt2 + i, (unsigned long) (addr + offs));
	}

	addr = (uint32_t *) CONFIG_SYS_SDRAM_BASE;
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

	printk_info("DRAM: simple test OK\n");
	return 0;
}

// Set the Vref mode for the controller
//
static void mctl_vrefzq_init(dram_para_t *para) {
	if (para->dram_tpr13 & (1 << 17))
		return;

	clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_IOVCR0), 0x7f7f7f7f, para->dram_tpr5);

	// IOCVR1
	if ((para->dram_tpr13 & (1 << 16)) == 0)
		clrsetbits_le32((MCTL_PHY_BASE + MCTL_PHY_IOVCR1), 0x7f, para->dram_tpr6 & 0x7f);
}

// Perform an init of the controller. This is actually done 3 times. The first
// time to establish the number of ranks and DQ width. The second time to
// establish the actual ram size. The third time is final one, with the final
// settings.
//
static int mctl_core_init(dram_para_t *para) {
	mctl_sys_init(para);

	mctl_vrefzq_init(para);

	mctl_com_init(para);

	mctl_phy_ac_remapping(para);

	mctl_set_timing_params(para);

	return mctl_channel_init(0, para);
}

/*
 * This routine sizes a DRAM device by cycling through address lines and
 * figuring out if they are connected to a real address line, or if the
 * address is a mirror.
 * First the column and bank bit allocations are set to low values (2 and 9
 * address lines). Then a maximum allocation (16 lines) is set for rows and
 * this is tested.
 * Next the BA2 line is checked. This seems to be placed above the column,
 * BA0-1 and row addresses. Finally, the column address is allocated 13 lines
 * and these are tested. The results are placed in dram_para1 and dram_para2.
 */
static int auto_scan_dram_size(dram_para_t *para) {
	uint32_t i = 0, j = 0, current_rank = 0;
	uint32_t rank_count = 1, addr_line = 0;
	uint32_t reg_val = 0, ret = 0, cnt = 0;
	unsigned long mc_work_mode;
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
		/* set row mode */
		clrsetbits_le32(mc_work_mode, 0xf0c, 0x6f0);
		sdelay(2);

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

		printk_debug("rank %d row = %d \n", current_rank, i);

		/* Store rows in para 1 */
		para->dram_para1 &= ~(0xffU << (16 * current_rank + 4));
		para->dram_para1 |= (i << (16 * current_rank + 4));
		printk_debug("para->dram_para1 = 0x%x\n", para->dram_para1);

		/* Set bank mode for current rank */
		if (current_rank == 1) { /* Set bank mode for rank0 */
			clrsetbits_le32((MCTL_COM_BASE + MCTL_COM_WORK_MODE0), 0xffc, 0x6a4);
		}

		/* Set bank mode for current rank */
		clrsetbits_le32(mc_work_mode, 0xffc, 0x6a4);
		sdelay(1);

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
		printk_debug("rank %d bank = %d \n", current_rank, (4 + i * 4));

		/* Store bank in para 1 */
		para->dram_para1 &= ~(0xfU << (16 * current_rank + 12));
		para->dram_para1 |= (i << (16 * current_rank + 12));
		printk_debug("para->dram_para1 = 0x%x\n", para->dram_para1);

		/* Set page mode for rank0 */
		if (current_rank == 1) {
			clrsetbits_le32(mc_work_mode, 0xffc, 0xaa0);
		}

		/* Set page mode for current rank */
		clrsetbits_le32(mc_work_mode, 0xffc, 0xaa0);
		sdelay(2);

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

		printk_debug("rank %d page size = %d KB \n", current_rank, i);

		/* Store page in para 1 */
		para->dram_para1 &= ~(0xfU << (16 * current_rank));
		para->dram_para1 |= (i << (16 * current_rank));
		printk_debug("para->dram_para1 = 0x%x\n", para->dram_para1);
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

/*
 * This routine sets up parameters with dqs_gating_mode equal to 1 and two
 * ranks enabled. It then configures the core and tests for 1 or 2 ranks and
 * full or half DQ width. It then resets the parameters to the original values.
 * dram_para2 is updated with the rank and width findings.
 */
static int auto_scan_dram_rank_width(dram_para_t *para) {
	uint32_t s1 = para->dram_tpr13;
	uint32_t s2 = para->dram_para1;

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

/*
 * This routine determines the SDRAM topology. It first establishes the number
 * of ranks and the DQ width. Then it scans the SDRAM address lines to establish
 * the size of each rank. It then updates dram_tpr13 to reflect that the sizes
 * are now known: a re-init will not repeat the autoscan.
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

static int init_DRAM(int type, dram_para_t *para) {
	uint32_t rc, mem_size_mb;

	printk_debug("DRAM BOOT DRIVE INFO: %s\n", "V0.24");
	printk_debug("DRAM CLK = %d MHz\n", para->dram_clk);
	printk_debug("DRAM Type = %d (2:DDR2,3:DDR3)\n", para->dram_type);
	if ((para->dram_odt_en & 0x1) == 0)
		printk_debug("DRAMC read ODT off\n");
	else
		printk_debug("DRAMC ZQ value: 0x%x\n", para->dram_zq);

	/* Test ZQ status */
	if (para->dram_tpr13 & (1 << 16)) {
		printk_debug("DRAM only have internal ZQ\n");
		setbits_le32((SYS_CONTROL_REG_BASE + ZQ_CAL_CTRL_REG), (1 << 8));
		writel(0, (SYS_CONTROL_REG_BASE + ZQ_RES_CTRL_REG));
		sdelay(10);
	} else {
		clrbits_le32((SYS_CONTROL_REG_BASE + ZQ_CAL_CTRL_REG), 0x3);
		writel(para->dram_tpr13 & (1 << 16), (R_PRCM_BASE + ANALOG_PWROFF_GATING_REG));
		sdelay(10);
		clrsetbits_le32((SYS_CONTROL_REG_BASE + ZQ_CAL_CTRL_REG), 0x108, (1 << 1));
		sdelay(10);
		setbits_le32((SYS_CONTROL_REG_BASE + ZQ_CAL_CTRL_REG), (1 << 0));
		sdelay(20);
		printk_debug("ZQ value = 0x%x\n", readl((SYS_CONTROL_REG_BASE + ZQ_RES_STATUS_REG)));
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
		printk_debug("DRAM ODT value: 0x%x\n", rc);

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
		rc = dramc_get_dram_size();
		printk_info("DRAM: size = %dMB\n", rc);
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
		if ((readl(0x070901f8) & (1 << 16)) || dramc_simple_wr_test(mem_size_mb, 4096))
			return 0;
	}

	return mem_size_mb;
}

uint32_t sunxi_dram_init(void *para) {
	dram_para_t *dram_para = (dram_para_t *) para;
	return init_DRAM(0, dram_para);
};