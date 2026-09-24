/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "psram-sun252iw2: " fmt

/**
 * @file psram-sun252iw2.c
 * @brief Native PSRAM initialization for the sun252iw2 SoC.
 *
 * Contains the reconstructed LPSRAM boot sequence and the Sunxi PSRAM
 * framework glue needed by the sun252iw2 platform.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <dt2c/driver.h>
#include <drivers/clk/clk.h>
#include <drivers/psram/psram.h>
#include <drivers/pmu/axp.h>

#include <common.h>

/* ------------------------------------------------------------------------ */
/* Register map (from the original binary)                                   */
/* ------------------------------------------------------------------------ */

#define LPSRAM_CCU_BASE			0x02001000
#define LPSRAM_CCU_PLL_LDRAM		0x02001010	/* PLL_LDRAM_CTRL */
#define LPSRAM_CCU_REG_1800		0x02001800
#define LPSRAM_CCU_REG_1808		0x02001808
#define LPSRAM_CCU_REG_180c		0x0200180c
#define LPSRAM_CCU_REG_1500		0x02001500
#define LPSRAM_CCU_REG_1540		0x02001540
#define LPSRAM_CCU_REG_1544		0x02001544

#define SYSCFG_BASE			0x03000000
#define SYSCFG_REG_150			0x03000150	/* voltage trim */
#define SYSCFG_REG_160			0x03000160
#define SYSCFG_REG_174			0x03000174
#define SYSCFG_REG_1f0			0x030001f0

#define SID_PWR_BASE			0x03090000
#define SID_EFUSE_BASE			0x03006000

#define LPSRAM_MCTL_BASE		0x02052000
#define LPSRAM_MCTL_2000		0x02052000
#define LPSRAM_MCTL_2004		0x02052004
#define LPSRAM_MCTL_200c		0x0205200c
#define LPSRAM_MCTL_2100		0x02052100
#define LPSRAM_MCTL_2118		0x02052118
#define LPSRAM_MCTL_211c		0x0205211c
#define LPSRAM_MCTL_2124		0x02052124
#define LPSRAM_MCTL_212c		0x0205212c
#define LPSRAM_MCTL_2130		0x02052130
#define LPSRAM_MCTL_2138		0x02052138
#define LPSRAM_MCTL_2150		0x02052150
#define LPSRAM_MCTL_2154		0x02052154
#define LPSRAM_MCTL_2180		0x02052180
#define LPSRAM_MCTL_2200		0x02052200
#define LPSRAM_MCTL_2204		0x02052204
#define LPSRAM_MCTL_220c		0x0205220c	/* MR command    */
#define LPSRAM_MCTL_2210		0x02052210	/* MR command data */
#define LPSRAM_MCTL_2104		0x02052104	/* MR trigger    */
#define LPSRAM_MCTL_2108		0x02052108	/* MR busy       */

#define LPSRAM_PHY_BASE			0x03103000
#define LPSRAM_PHY_3000			0x03103000
#define LPSRAM_PHY_3008			0x03103008
#define LPSRAM_PHY_300c			0x0310300c
#define LPSRAM_PHY_3010			0x03103010
#define LPSRAM_PHY_3018			0x03103018
#define LPSRAM_PHY_3028			0x03103028
#define LPSRAM_PHY_3044			0x03103044
#define LPSRAM_PHY_3080			0x03103080
#define LPSRAM_PHY_308c			0x0310308c
#define LPSRAM_PHY_30c0			0x031030c0
#define LPSRAM_PHY_3100			0x03103100
#define LPSRAM_PHY_3108			0x03103108
#define LPSRAM_PHY_3110			0x03103110
#define LPSRAM_PHY_3114			0x03103114
#define LPSRAM_PHY_3140			0x03103140
#define LPSRAM_PHY_3180			0x03103180
#define LPSRAM_PHY_3188			0x03103188
#define LPSRAM_PHY_3200			0x03103200
#define LPSRAM_PHY_3300			0x03103300
#define LPSRAM_PHY_3308			0x03103308
#define LPSRAM_PHY_3310			0x03103310
#define LPSRAM_PHY_3370			0x03103370
#define LPSRAM_PHY_3388			0x03103388

#define LPSRAM_MAST_BASE		0x03102000
#define LPSRAM_MAST_2000		0x03102000
#define LPSRAM_MAST_2008			0x03102008
#define LPSRAM_MAST_200c		0x0310200c
#define LPSRAM_MAST_2014		0x03102014
#define LPSRAM_MAST_2020		0x03102020
#define LPSRAM_MAST_2024		0x03102024
#define LPSRAM_MAST_2028		0x03102028
#define LPSRAM_MAST_2050		0x03102050

#define LPSRAM_REMAP_BASE		0x03102500

#define PSRAM_MEM_BASE			0x40000000

/* ------------------------------------------------------------------------ */
/* Initialisation parameter block (the "dram para" table passed to           */
/* lpsram_init(); layout matches the original binary exactly).               */
/* ------------------------------------------------------------------------ */

typedef struct {
	uint32_t dram_clk;		/* +0x00: target clock in MHz          */
	uint32_t dram_type;		/* +0x04: detected/auto-filled type     */
	uint32_t dram_zq;		/* +0x08                                */
	uint32_t dram_odt_en;		/* +0x0c                                */
	uint32_t dram_para1;		/* +0x10: [15:0] size MB                */
	uint32_t dram_para2;		/* +0x14: flags (dual die, width...)    */
	uint32_t dram_mr0;		/* +0x18 (used as 16-bit MR values)     */
	uint32_t dram_mr1;		/* +0x1c                                */
	uint32_t dram_mr2;		/* +0x20                                */
	uint32_t dram_mr3;		/* +0x24                                */
	uint32_t dram_tpr0;		/* +0x28                                */
	uint32_t dram_tpr1;		/* +0x2c                                */
	uint32_t dram_tpr2;		/* +0x30                                */
	uint32_t dram_tpr3;		/* +0x34: [7:0] SoC voltage trim        */
	uint32_t dram_tpr4;		/* +0x38: UI delay training result      */
	uint32_t dram_tpr5;		/* +0x3c: ZQ vref (0 -> 0x48484848)     */
	uint32_t dram_tpr6;		/* +0x40: ZQ param (0 -> 72)            */
	uint32_t dram_tpr7;		/* +0x44                                */
	uint32_t dram_tpr8;		/* +0x48                                */
	uint32_t dram_tpr9;		/* +0x4c                                */
	uint32_t dram_tpr10;		/* +0x50: mode flags (bit19: train request, bit31: trained) */
	uint32_t dram_tpr11;		/* +0x54: RDQ window nibbles            */
	uint32_t dram_tpr12;		/* +0x58: WDQ window nibbles            */
	uint32_t dram_tpr13;		/* +0x5c: control flags                 */
	uint8_t rdq_delay[32];		/* +0x60: per-byte RDQ delays           */
	uint8_t wdq_delay[32];		/* +0x80: per-byte WDQ delays           */
} lpsram_para_t;

#define PSRAM_TYPE_APS3208K	1
#define PSRAM_TYPE_APS64M	2
#define PSRAM_TYPE_APS128M	3
#define PSRAM_TYPE_APS3208E	4
#define PSRAM_TYPE_W955		5
#define PSRAM_TYPE_W956		6
#define PSRAM_TYPE_W957		7
#define PSRAM_TYPE_W958		8

struct dram_type_entry {
	uint32_t type;
	const char *name;
};

static const struct dram_type_entry dram_type_table[8] = {
	{ PSRAM_TYPE_APS3208K, "APS3208K" },
	{ PSRAM_TYPE_APS64M,   "APS64M"   },
	{ PSRAM_TYPE_APS128M,  "APS128M"  },
	{ PSRAM_TYPE_APS3208E, "APS3208E" },
	{ PSRAM_TYPE_W955,     "W955"     },
	{ PSRAM_TYPE_W956,     "W956"     },
	{ PSRAM_TYPE_W957,     "W957"     },
	{ PSRAM_TYPE_W958,     "W958"     },
};

static uint8_t dram_wdqbit[32];	/* trained WDQ bit delays  */
static uint8_t dram_rdqbit[32];	/* trained RDQ bit delays  */
uint32_t m_early_terminate_flag;
static uint32_t Seed = 1;

uint32_t dst_from_none(lpsram_para_t *para);
uint32_t dst_dq_eye_scan(lpsram_para_t *para);
uint32_t lpsram_phy_dx_ui_delay_training(lpsram_para_t *para);

/* ------------------------------------------------------------------------ */
/* Small helpers                                                             */
/* ------------------------------------------------------------------------ */

void lpsram_udelay(uint32_t us)
{
	(void)us;	/* original object contains an empty stub */
}

void FlushDcacheAll(void)
{
	flush_dcache_all();
}

const char *get_dram_type_name(uint32_t type)
{
	size_t i;

	for (i = 0; i < 8; i++) {
		if (dram_type_table[i].type == type)
			return dram_type_table[i].name;
	}
	return "Unknown";
}

void sid_pwr_ctrl(void)
{
	writel(0x80000000, SID_PWR_BASE + 0x310);
	writel(1, SID_PWR_BASE + 0x204);
}

uint32_t sid_read_key(uint32_t key)
{
	uint32_t val;

	sid_pwr_ctrl();
	writel(key >> 2, SID_EFUSE_BASE + 4);
	val = readl(SID_EFUSE_BASE) & 0x0000fffc;
	writel(val | 0xadbf0002, SID_EFUSE_BASE);
	while (readl(SID_EFUSE_BASE) & 2)
		;
	writel(val, SID_EFUSE_BASE);
	return readl(SID_EFUSE_BASE + 12);
}

/* ------------------------------------------------------------------------ */
/* Type detection from the SoC eFuse chip-id                                 */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_get_type_by_hwid(void)
{
	uint32_t chipid_lo = sid_read_key(0) & 0xffff;
	uint32_t variant = sid_read_key(28) >> 26;

	if (chipid_lo == 0x4100) {		/* F101S2 */
		if (variant == 6)
			return PSRAM_TYPE_APS3208K;
		if (variant == 7)
			return PSRAM_TYPE_W955;
		printf_dram("[ERROR] Not supported PSRAM type of F101S2: %d\n", variant);
		return 0;
	}
	if (chipid_lo == 0x4300) {		/* F101S3 */
		if (variant == 4)
			return PSRAM_TYPE_W957;
		if (((variant - 3) & ~3u) == 0)	/* variant == 3 or 4 */
			return PSRAM_TYPE_APS128M;
		printf_dram("[ERROR] Not supported PSRAM type of F101S3: %d\n", variant);
		return 0;
	}
	printf_dram("Unknown error: 0x%04x\n", chipid_lo);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* Parameter sanitising per PSRAM type                                       */
/* ------------------------------------------------------------------------ */

int lpsram_para_merge_config(lpsram_para_t *para)
{
	uint32_t width;
	uint32_t type;

	if (para == NULL)
		return 1;

	width = para->dram_para1 & 0xffff;
	type = para->dram_type;

	switch (type) {
	case 1:
		if ((para->dram_clk - 1) > 232)
			para->dram_clk = 233;
		if (para->dram_zq == 0)
			para->dram_zq = 0x7b7bfb;
		if ((width - 1) > 7)
			width = 8;
		break;
	case 3:
		if ((para->dram_clk - 1) > 251)
			para->dram_clk = 252;
		if (para->dram_zq == 0)
			para->dram_zq = 0x7bfbfb;
		if ((width - 1) > 15)
			width = 16;
		para->dram_tpr10 &= 0xf000ffff;
		dcache_disable();
		break;
	case 5:
		if ((para->dram_clk - 1) > 232)
			para->dram_clk = 233;
		if (para->dram_zq == 0)
			para->dram_zq = 0x7b7bfb;
		if (para->dram_clk > 200)
			para->dram_zq = 0x7bfbfb;
		if ((width - 1) > 7)
			width = 8;
		break;
	case 7:
		if ((para->dram_clk - 1) > 251)
			para->dram_clk = 252;
		if (para->dram_zq == 0)
			para->dram_zq = 0x7b7bfb;
		if ((width - 1) > 15)
			width = 16;
		break;
	default:
		printf_dram("[ERROR] Not supported PSRAM type: %d\n", type);
		return 1;
	}

	para->dram_para1 = (para->dram_para1 & 0xffff0000) | (width & 0xffff);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* Voltage / master access / controller reset                                */
/* ------------------------------------------------------------------------ */

uint32_t dram_vol_set(lpsram_para_t *para)
{
	uint8_t vol = ((const volatile uint8_t *)para)[52];

	if (vol != 0) {
		uint32_t v = readl(SYSCFG_REG_150);

		writel((v & 0xffffff00) | vol, SYSCFG_REG_150);
	}
	return readl(SYSCFG_REG_150);
}

void lpsram_enable_all_master(void)
{
	writel(0xffffffff, LPSRAM_MAST_2020);
	writel(0x000000ff, LPSRAM_MAST_2024);
	writel(0x0000ffff, LPSRAM_MAST_2028);
}

void lpsram_disable_all_master(void)
{
	writel(1, LPSRAM_MAST_2020);
	writel(0, LPSRAM_MAST_2024);
	writel(0, LPSRAM_MAST_2028);
}

void lpsram_reset_cfg(void)
{
	writel(0xffff0000, LPSRAM_MCTL_2210);
	writel(0, LPSRAM_MCTL_220c);
	writel((readl(LPSRAM_MCTL_2104) & ~0xfu) | 0xd, LPSRAM_MCTL_2104);
}

/* ------------------------------------------------------------------------ */
/* Mode-register access                                                      */
/* ------------------------------------------------------------------------ */

static void lpsram_mr_trigger(uint32_t trig_val)
{
	uint32_t v = readl(LPSRAM_MCTL_2104);

	v = (v & ~0xfu) | trig_val;
	writel(v, LPSRAM_MCTL_2104);
	while (readl(LPSRAM_MCTL_2108) & 1)
		;
}

void lpsram_mr_write(lpsram_para_t *para, uint32_t mreg, uint32_t value, uint32_t hi)
{
	switch (para->dram_type) {
	case 1:
		writel(0xc0000000 | (mreg & 0xff), LPSRAM_MCTL_2210);
		writel(value << 24, LPSRAM_MCTL_220c);
		break;
	case 2:
	case 3:
		writel(0xc0c00000, LPSRAM_MCTL_2210);
		writel(((value << 8) & 0xffff) | ((mreg << 16) & 0xff0000),
		       LPSRAM_MCTL_220c);
		break;
	case 4:
		writel(0xc0c00000 | ((mreg << 24) & 0x07000000) |
		       ((mreg << 16) & 0x00070000) | (value & 0xff),
		       LPSRAM_MCTL_2210);
		writel(0, LPSRAM_MCTL_220c);
		break;
	default:	/* types 5..8 */
		writel(0x60000100, LPSRAM_MCTL_2210);
		writel(((hi << 8) & 0xffff) | (value & 0xff) | ((mreg << 16) & 0xff0000),
		       LPSRAM_MCTL_220c);
		break;
	}
	lpsram_mr_trigger(0xd);
}

uint32_t lpsram_mr_read(lpsram_para_t *para, uint32_t mreg)
{
	switch (para->dram_type) {
	case 1:
		writel(0x40000000 | (mreg & 0xff), LPSRAM_MCTL_2210);
		writel(0, LPSRAM_MCTL_220c);
		break;
	case 2:
	case 3:
		writel(0x40400000, LPSRAM_MCTL_2210);
		writel((mreg << 16) & 0xff0000, LPSRAM_MCTL_220c);
		break;
	case 4:
		writel(0x40400000 | ((mreg << 24) & 0x07000000) |
		       ((mreg << 16) & 0x00070000), LPSRAM_MCTL_2210);
		writel(0, LPSRAM_MCTL_220c);
		break;
	default:	/* types 5..8 */
		writel(0xe0000100, LPSRAM_MCTL_2210);
		writel((mreg << 16) & 0xff0000, LPSRAM_MCTL_220c);
		break;
	}
	lpsram_mr_trigger(0xf);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* PLL_LDRAM clock configuration                                             */
/* ------------------------------------------------------------------------ */

uint32_t ccm_set_pll_ldram_clk(uint32_t clk)
{
	if (clk > 200) {
		uint32_t n = clk / 24;
		uint32_t v;

		v = readl(LPSRAM_CCU_PLL_LDRAM);
		v = (v & 0xf7ff00fc) | ((n - 1) << 8) | 0xc0000000;
		writel(v, LPSRAM_CCU_PLL_LDRAM);

		v = readl(LPSRAM_CCU_PLL_LDRAM);
		writel(v & ~(1u << 29), LPSRAM_CCU_PLL_LDRAM);
		v = readl(LPSRAM_CCU_PLL_LDRAM);
		writel(v | (1u << 29), LPSRAM_CCU_PLL_LDRAM);

		while (!(readl(LPSRAM_CCU_PLL_LDRAM) & 0x10000000))
			;

		v = readl(LPSRAM_CCU_PLL_LDRAM);
		writel(v | (1u << 27), LPSRAM_CCU_PLL_LDRAM);

		v = readl(LPSRAM_CCU_REG_1800);
		v = (v & 0xf8fffcfc) | 0x81000000;
		writel(v, LPSRAM_CCU_REG_1800);

		return n * 24;
	}

	/* Unreachable with the parameter-merge clamps (clk >= 233). */
	writel(0xffffffff, LPSRAM_CCU_REG_1800);
	__asm__ volatile("ebreak");
	return 0;
}

/* ------------------------------------------------------------------------ */
/* System-level (clock/bus) bring-up                                         */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_sys_init_cfg(lpsram_para_t *para)
{
	uint32_t clk = para->dram_clk;
	uint32_t v;

	v = readl(SYSCFG_REG_1f0);
	writel(v | 1, SYSCFG_REG_1f0);

	writel(readl(LPSRAM_CCU_REG_1808) & 0xfffeffff, LPSRAM_CCU_REG_1808);

	writel(readl(LPSRAM_CCU_REG_1544) & ~0x80000000u, LPSRAM_CCU_REG_1544);
	writel(readl(LPSRAM_CCU_REG_1540) & 0xbfffffff, LPSRAM_CCU_REG_1540);
	writel(readl(LPSRAM_CCU_REG_180c) & ~1u, LPSRAM_CCU_REG_180c);
	writel(readl(LPSRAM_CCU_REG_180c) & 0xfffeffff, LPSRAM_CCU_REG_180c);
	writel(readl(LPSRAM_CCU_REG_1800) & 0xbfffffff, LPSRAM_CCU_REG_1800);
	writel(readl(LPSRAM_CCU_REG_1800) & ~0x80000000u, LPSRAM_CCU_REG_1800);
	writel(readl(LPSRAM_CCU_REG_1800) | (1u << 27), LPSRAM_CCU_REG_1800);

	/*
	 * dram_clk is the controller target: the original doubles it for the PLL
	 * request and halves the achieved PLL back down (PLL runs on a 24 MHz
	 * grid, so 2*252 -> PLL 504 -> controller 252).
	 */
	para->dram_clk = ccm_set_pll_ldram_clk(clk * 2) >> 1;

	lpsram_disable_all_master();

	writel(readl(LPSRAM_CCU_REG_1544) | 0x03000001, LPSRAM_CCU_REG_1544);
	writel(readl(LPSRAM_CCU_REG_180c) | (1u << 16), LPSRAM_CCU_REG_180c);
	writel(readl(LPSRAM_CCU_REG_1540) | (1u << 30), LPSRAM_CCU_REG_1540);
	writel(readl(LPSRAM_CCU_REG_1800) | (1u << 30), LPSRAM_CCU_REG_1800);
	writel(readl(LPSRAM_CCU_REG_1808) | (1u << 16), LPSRAM_CCU_REG_1808);
	writel(readl(LPSRAM_CCU_REG_180c) | 1, LPSRAM_CCU_REG_180c);
	writel(readl(LPSRAM_CCU_REG_1544) | 0x80000000, LPSRAM_CCU_REG_1544);
	writel(readl(LPSRAM_CCU_REG_1800) | 0x80000000, LPSRAM_CCU_REG_1800);
	writel(readl(LPSRAM_CCU_REG_1800) | (1u << 27), LPSRAM_CCU_REG_1800);

	v = readl(LPSRAM_PHY_300c);
	v = (v & ~0x20u) | 0x40;
	writel(v, LPSRAM_PHY_300c);
	writel(readl(LPSRAM_PHY_300c) | (1u << 15), LPSRAM_PHY_300c);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* Per-bit read/write delay programming                                      */
/* ------------------------------------------------------------------------ */

void lpsram_phy_dx_bit_delay_compensation(lpsram_para_t *para)
{
	uint32_t tpr10 = para->dram_tpr10;
	uint32_t tpr11 = para->dram_tpr11;
	uint32_t tpr12 = para->dram_tpr12;
	uint32_t v;
	int i;

	v = readl(LPSRAM_PHY_3100);
	writel(v & ~(1u << 26), LPSRAM_PHY_3100);

	if (tpr10 & 0x100000) {
		/* Delay tables supplied as byte arrays in the parameter block. */
		for (i = 0; i < 32; i++) {
			dram_wdqbit[i] = para->wdq_delay[i];
			dram_rdqbit[i] = para->rdq_delay[i];
		}
	} else {
		/* Nibble-packed tables in tpr11 (RDQ) / tpr12 (WDQ). */
		for (i = 0; i < 32; i++) {
			int sh = (i / 8) * 4;
			dram_wdqbit[i] = (tpr11 >> sh) & 0xf;
			dram_rdqbit[i] = (tpr12 >> sh) & 0xf;
		}
	}

	for (i = 0; i < 8; i++) {
		v = readl(LPSRAM_PHY_3310 + 4 * i);
		v = (v & 0xffffc0c0) |
		    ((uint32_t)dram_rdqbit[i] << 1 & 0x1e) |
		    ((uint32_t)dram_wdqbit[i] << 9 & 0x1e00);
		writel(v, LPSRAM_PHY_3310 + 4 * i);
	}
	for (i = 0; i < 16; i++) {
		v = readl(LPSRAM_PHY_3370 + 4 * i);
		v = (v & 0xffffc0c0) |
		    ((uint32_t)dram_rdqbit[i] << 1 & 0x1e) |
		    ((uint32_t)dram_wdqbit[i] << 9 & 0x1e00);
		writel(v, LPSRAM_PHY_3370 + 4 * i);
	}

	v = readl(LPSRAM_PHY_3100);
	writel(v & ~(1u << 26), LPSRAM_PHY_3100);

	v = readl(LPSRAM_PHY_3300 + 52);
	writel((v & 0xffffc0c0) | ((tpr11 << 9) & 0x1e00), LPSRAM_PHY_3300 + 52);
	v = readl(LPSRAM_PHY_3300 + 56);
	writel((v & 0xffffc0c0) | ((tpr11 << 9) & 0x1e00), LPSRAM_PHY_3300 + 56);
	v = readl(0x03103380 + 52);
	writel((v & 0xffffc0c0) | ((tpr11 << 5) & 0x1e00), 0x03103380 + 52);
	v = readl(0x03103380 + 56);
	writel((v & 0xffffc0c0) | ((tpr11 << 5) & 0x1e00), 0x03103380 + 56);
	v = readl(LPSRAM_PHY_3300 + 52);
	writel((v & 0xffffc0c0) | ((tpr12 >> 15) & 0x1e), LPSRAM_PHY_3300 + 52);
	v = readl(LPSRAM_PHY_3300 + 56);
	writel((v & 0xffffc0c0) | ((tpr12 >> 19) & 0x1e), LPSRAM_PHY_3300 + 56);
	v = readl(0x03103380 + 52);
	writel((v & 0xffffc0c0) | ((tpr12 >> 19) & 0x1e), 0x03103380 + 52);
	v = readl(0x03103380 + 56);
	writel((v & 0xffffc0c0) | ((tpr12 >> 19) & 0x1e), 0x03103380 + 56);

	switch (para->dram_type) {
	case 1:
	case 5:
		v = readl(0x02053058);
		v = (v & 0x8000007f) |
		    ((tpr11 >> 8) & 0xf00) |
		    ((tpr10 << 12) & 0x00f00000) |
		    ((tpr11 >> 6) & 0x0003c000) |
		    ((tpr10 << 14) & 0x3c000000);
		writel(v, 0x02053058);
		break;
	case 3:
	case 7:
	case 8:
		v = readl(0x02053058);
		v = (v & 0x8000007f) |
		    ((tpr10 << 12) & 0x00f00000) |
		    ((tpr11 >> 8) & 0xf00) |
		    ((tpr11 >> 20 << 8) & 0xf00);
		writel(v, 0x02053058);
		break;
	default:
		printf_dram("[ERROR] Not supported PSRAM type!\n");
		break;
	}

	/*
	 * Plain ORs into the DX-block +0x3c fields (bits 25..28), no
	 * clear-mask: the original leaves the remaining bits untouched.
	 */
	v = readl(LPSRAM_PHY_3300 + 0x3c);
	writel(v | ((tpr11 << 9) & 0x1e000000), LPSRAM_PHY_3300 + 0x3c);
	v = readl(0x03103380 + 0x3c);
	writel(v | ((tpr11 << 5) & 0x1e000000), 0x03103380 + 0x3c);

	v = readl(LPSRAM_PHY_3100);
	writel(v | 0x06000000, LPSRAM_PHY_3100);

	v = (tpr10 << 4) & 0xf00;
	{
		uint32_t r;
		for (r = 0x03103240; r < 0x0310327c; r += 4)
			writel(readl(r) | v, r);
		for (r = 0x03103228; r < 0x03103240; r += 4)
			writel(readl(r) | v, r);
	}

	v = readl(LPSRAM_PHY_3200 + 0x18);
	writel(v | ((tpr10 << 8) & 0xf00), LPSRAM_PHY_3200 + 0x18);
	v = readl(LPSRAM_PHY_3200 + 0x1c);
	writel(v | (tpr10 & 0xf00), LPSRAM_PHY_3200 + 0x1c);
	v = readl(LPSRAM_PHY_3200 + 0x80);
	writel(v | ((tpr10 >> 4) & 0xf00), LPSRAM_PHY_3200 + 0x80);
}

/* ------------------------------------------------------------------------ */
/* ZQ / VREF calibration                                                     */
/* ------------------------------------------------------------------------ */

void lpsram_vrefzq_init(lpsram_para_t *para)
{
	uint32_t tpr13 = para->dram_tpr13;
	uint32_t zq;
	uint32_t v;

	if (tpr13 & 0x20000)
		return;

	zq = readl(LPSRAM_PHY_3110) & 0x80808080;
	v = para->dram_tpr5;
	if (v == 0)
		v = 0x48484848;
	writel(v | zq, LPSRAM_PHY_3110);

	if (tpr13 & 0x10000)
		return;

	v = readl(LPSRAM_PHY_3114);
	zq = para->dram_tpr6 & 0x7f;
	if (zq == 0)
		zq = 72;
	writel((v & 0xffffff80) | zq, LPSRAM_PHY_3114);
}

/* ------------------------------------------------------------------------ */
/* Data-byte remapping                                                       */
/* ------------------------------------------------------------------------ */

void lpsram_db_remap_config(uint32_t mask, const uint32_t *src, const uint32_t *dst)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (mask & (1u << i)) {
			writel(src[i], LPSRAM_REMAP_BASE + 0x10 + 8 * i);
			writel(dst[i], LPSRAM_REMAP_BASE + 0x14 + 8 * i);
		}
	}

	writel((readl(LPSRAM_REMAP_BASE) & ~0x1eu) | ((mask << 1) & 0x1e),
	       LPSRAM_REMAP_BASE);
}

/* ------------------------------------------------------------------------ */
/* MSI PHY initialisation                                                    */
/* ------------------------------------------------------------------------ */

static const uint32_t db_remap_type7_src[4] = { 0x10326475, 0x10325476, 0, 0 };
static const uint32_t db_remap_type8_src[4] = { 0x67452301, 0x67452301, 0, 0 };
static const uint32_t db_remap_dst[4]       = { 8, 8, 8, 8 };

uint32_t msi_phy_initial_cfg(lpsram_para_t *para)
{
	uint32_t para2 = para->dram_para2;
	uint32_t zq_status;
	uint32_t v;

	v = readl(LPSRAM_MAST_2008);
	writel((v & 0xffffc0ff) | (1u << 12), LPSRAM_MAST_2008);

	writel(0x2020, LPSRAM_MAST_2014);

	v = readl(LPSRAM_PHY_300c);
	writel(v | 0xa020, LPSRAM_PHY_300c);

	v = readl(LPSRAM_MAST_200c);
	writel((v & 0xfffff000) | 399, LPSRAM_MAST_200c);

	v = readl(LPSRAM_MAST_2000);
	v &= 0xffff8fff;
	if (para2 & 0x10) {
		v |= (para2 & 1) ? 0x2000 : 0x3000;
	} else {
		v |= (para2 << 13) & 0x2000;
	}
	v |= 1u << 27;
	writel(v, LPSRAM_MAST_2000);

	v = readl(LPSRAM_PHY_3044);
	writel((v & 0xffffffc0) | 0xc3, LPSRAM_PHY_3044);

	v = readl(0x03103208);
	writel((v & 0xfff80037) | (1u << 16), 0x03103208);

	lpsram_phy_dx_bit_delay_compensation(para);

	v = readl(LPSRAM_PHY_3108);		/* DX0 block +0x08 */
	writel((v & 0xfffff03f) | 0x380, LPSRAM_PHY_3108);
	v = readl(LPSRAM_PHY_3080 + 0x3c);
	writel((v & ~7u) | 0x104, LPSRAM_PHY_3080 + 0x3c);
	v = readl(LPSRAM_PHY_3100 + 0x1c);
	writel(v & 0xffffff, LPSRAM_PHY_3100 + 0x1c);

	v = readl(LPSRAM_PHY_3140);
	v = (v & 0xf8000000) |
	    ((para->dram_zq & 0xffffff) ? (para->dram_zq & 0xffffff) : 0x3b3bbb);
	v |= 1u << 25;
	writel(v, LPSRAM_PHY_3140);
	writel(0, 0x03103444);
	writel(0, 0x031034c4);

	if (para->dram_type == PSRAM_TYPE_W957) {
		uint32_t tmp[8];

		memcpy(tmp, db_remap_type7_src, 16);
		memcpy(tmp + 4, db_remap_dst, 16);
		lpsram_db_remap_config(1, tmp, tmp + 4);
	}
	if (para->dram_type == PSRAM_TYPE_W958) {
		uint32_t tmp[8];

		memcpy(tmp, db_remap_type8_src, 16);
		memcpy(tmp + 4, db_remap_dst, 16);
		lpsram_db_remap_config(3, tmp, tmp + 4);
	}

	v = readl(LPSRAM_PHY_30c0);
	writel((v & 0xf0000000) | 0x1003087, LPSRAM_PHY_30c0);

	v = readl(LPSRAM_PHY_3000);
	writel(v | 98, LPSRAM_PHY_3000);
	writel(v | 99, LPSRAM_PHY_3000);

	while (!(readl(LPSRAM_PHY_3010) & 1))
		;
	v = readl(LPSRAM_PHY_3010);
	/*
	 * ZQ status bits [27:20]: 0 = calibrated; nonzero with bit20 set is
	 * the hard "external resistor" error (print and bail out without the
	 * tail).  Nonzero without bit20 still runs the tail but reports
	 * failure to the caller, matching the original.
	 */
	zq_status = (v >> 20) & 0xff;
	if (zq_status != 0 && (v & 0x100000)) {
		printf_dram("ZQ calibration error,check external 240 ohm resistor.\n");
		return 0;
	}

	while (!(readl(LPSRAM_PHY_3018) & 1))
		;

	v = readl(LPSRAM_PHY_3080 + 0xc);
	writel(v | 0x80000000, LPSRAM_PHY_3080 + 0xc);
	writel(readl(LPSRAM_PHY_3080 + 0xc) & ~0x80000000u, LPSRAM_PHY_3080 + 0xc);
	writel(readl(LPSRAM_MAST_2014) | 0x80000000, LPSRAM_MAST_2014);

	v = readl(LPSRAM_PHY_3100 + 0xc);
	writel(v & 0xf9ffffff, LPSRAM_PHY_3100 + 0xc);

	v = readl(LPSRAM_MAST_2050);
	v &= ~(((readl(LPSRAM_MAST_2000) >> 12) & 2));
	writel(v, LPSRAM_MAST_2050);

	return zq_status == 0;
}

/* ------------------------------------------------------------------------ */
/* Controller initial configuration (type/clock dependent timing tables)     */
/* ------------------------------------------------------------------------ */

static uint8_t mr_or_default(uint32_t v, uint8_t dflt)
{
	uint8_t lo = (uint8_t)v;

	return lo ? lo : dflt;
}

static uint8_t mr_hi_or_default(uint32_t v, uint8_t dflt)
{
	uint8_t hi = (uint8_t)(v >> 8);

	return hi ? hi : dflt;
}

/* Epilogue shared by all branches (.L149 of the original). */
static void lpsram_controller_initial_finish(lpsram_para_t *para)
{
	uint32_t tpr2 = para->dram_tpr2;
	uint32_t val;

	writel(readl(LPSRAM_MCTL_212c) & ~8u, LPSRAM_MCTL_212c);
	val = ((tpr2 >> 8) & 0x7f) ? (tpr2 & 0x2f00) : 0xc00;
	writel((readl(LPSRAM_MCTL_2154) & 0xff000000) | val | 0x880005,
	       LPSRAM_MCTL_2154);
}

/* .L214 of the original: final 0x02052200 update before the epilogue. */
static void lpsram_final_2200_update(void)
{
	writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | 0x60000000, LPSRAM_MCTL_2200);
}

void lpsram_controller_initial_cfg(lpsram_para_t *para)
{
	uint32_t type = para->dram_type;
	uint32_t clk = para->dram_clk;

	/* Common preamble */
	writel(readl(0x0205308c) & 0xffffbbff, 0x0205308c);

	if (type == 1) {
		writel((readl(0x0205308c) & 0xff3f0000) | 0xff, 0x0205308c);
		writel(readl(LPSRAM_MCTL_2000) | 7, LPSRAM_MCTL_2000);
		writel((readl(LPSRAM_MCTL_200c) & 0x3ffffff) | 0x90000000, LPSRAM_MCTL_200c);
		writel(readl(LPSRAM_MCTL_2100) | 2, LPSRAM_MCTL_2100);
		writel((readl(LPSRAM_MCTL_2118) & 0xfffffff) | 0x80000000, LPSRAM_MCTL_2118);
		writel(readl(LPSRAM_MCTL_211c) & ~0xfu, LPSRAM_MCTL_211c);
		writel(readl(LPSRAM_MCTL_2124) & ~4u, LPSRAM_MCTL_2124);
		writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x20000800, LPSRAM_MCTL_2150);
		writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29), LPSRAM_MCTL_2200);
		writel((readl(0x02052214) & 0xffff8080) | 0x420, 0x02052214);
		writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 4, LPSRAM_MCTL_2130);

		lpsram_reset_cfg();
		lpsram_mr_write(para, 0, mr_or_default(para->dram_mr0, 63), 0);
		lpsram_mr_write(para, 4, mr_or_default(para->dram_mr1, 128), 0);
		lpsram_final_2200_update();
	} else if (type == 4) {
		writel((readl(0x0205308c) & 0x000ff000) | 0x00c000ff, 0x0205308c);
		writel(readl(LPSRAM_MCTL_2000) | 7, LPSRAM_MCTL_2000);
		writel((readl(LPSRAM_MCTL_2004) & ~0x300u) | 0x200, LPSRAM_MCTL_2004);
		writel((readl(LPSRAM_MCTL_200c) & 0x3ffffff) | 0x84000000, LPSRAM_MCTL_200c);
		writel(readl(LPSRAM_MCTL_2100) | 2, LPSRAM_MCTL_2100);
		writel((readl(LPSRAM_MCTL_2118) & 0xfffffff) | 0x80000000, LPSRAM_MCTL_2118);
		writel(readl(LPSRAM_MCTL_211c) & ~0xfu, LPSRAM_MCTL_211c);
		writel(readl(LPSRAM_MCTL_2124) & ~4u, LPSRAM_MCTL_2124);

		if (clk <= 204) {
			writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x18001800, LPSRAM_MCTL_2150);
			writel((readl(LPSRAM_MCTL_2200) & 0xff) | 0x101400, LPSRAM_MCTL_2200);
			writel((readl(LPSRAM_MCTL_2204) & 0xff) | 0x12121200, LPSRAM_MCTL_2204);
			writel((readl(0x02052214) & 0xffff8080) | 0x518, 0x02052214);
			writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 2, LPSRAM_MCTL_2130);
		} else if (clk <= 252) {
			writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x22002200, LPSRAM_MCTL_2150);
			writel((readl(LPSRAM_MCTL_2200) & 0xff) | 0x101e00, LPSRAM_MCTL_2200);
			writel((readl(LPSRAM_MCTL_2204) & 0xff) | 0x12121a00, LPSRAM_MCTL_2204);
			writel((readl(0x02052214) & 0xffff8080) | 0x522, 0x02052214);
			writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 4, LPSRAM_MCTL_2130);
		} else if (clk <= 408) {
			writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x2e002e00, LPSRAM_MCTL_2150);
			writel((readl(LPSRAM_MCTL_2200) & 0xff) | 0x102a00, LPSRAM_MCTL_2200);
			writel((readl(LPSRAM_MCTL_2204) & 0xff) | 0x12122700, LPSRAM_MCTL_2204);
			writel((readl(0x02052214) & 0xffff8080) | 0x52e, 0x02052214);
			writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 3, LPSRAM_MCTL_2130);
		}

		lpsram_reset_cfg();
		lpsram_mr_write(para, 6, mr_or_default(para->dram_mr0, 64), 0);
		lpsram_mr_read(para, 6);
		lpsram_final_2200_update();
	} else if (type == 2 || type == 3) {
		writel((readl(0x0205308c) & 0xff3f0000) | 0x00c00084, 0x0205308c);
		writel(readl(LPSRAM_MCTL_2000) | 7, LPSRAM_MCTL_2000);
		writel((readl(LPSRAM_MCTL_200c) & 0x3ffffff) | 0x80000000, LPSRAM_MCTL_200c);
		writel(readl(LPSRAM_MCTL_2100) | 2, LPSRAM_MCTL_2100);
		writel(0x8d95e4cf, LPSRAM_MCTL_2118);
		writel(readl(LPSRAM_MCTL_211c) & ~0xfu, LPSRAM_MCTL_211c);
		writel(readl(LPSRAM_MCTL_2124) & ~4u, LPSRAM_MCTL_2124);

		if (type == 2) {
			writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x1e001200, LPSRAM_MCTL_2150);
			writel((readl(LPSRAM_MCTL_2204) & 0xe0ffffff) | 0x12000000, LPSRAM_MCTL_2204);
			writel((readl(0x02052214) & 0xffff8080) | 0x50e, 0x02052214);
			lpsram_reset_cfg();
			lpsram_mr_write(para, 0, mr_or_default(para->dram_mr0, 49), 0);
			lpsram_mr_read(para, 0);
			lpsram_mr_write(para, 4, mr_or_default(para->dram_mr1, 32), 0);
			lpsram_mr_read(para, 4);
			lpsram_mr_write(para, 8, mr_or_default(para->dram_mr2, 3), 0);
			lpsram_mr_read(para, 8);
			writel((readl(LPSRAM_MCTL_2200) & 0xfffffff) | (1u << 29),
			       LPSRAM_MCTL_2200);
		} else {
			if (clk <= 204) {
				writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x20001200,
				       LPSRAM_MCTL_2150);
				writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29),
				       LPSRAM_MCTL_2200);
				writel((readl(0x02052214) & 0xffff8080) | 0x512, 0x02052214);
			} else {
				writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 4, LPSRAM_MCTL_2130);
				writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x28001600,
				       LPSRAM_MCTL_2150);
				writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29),
				       LPSRAM_MCTL_2200);
				writel((readl(0x02052214) & 0xffff8080) | 0x516, 0x02052214);
			}
			lpsram_reset_cfg();
			lpsram_mr_write(para, 0, mr_or_default(para->dram_mr0, clk <= 204 ? 49 : 57), 0);
			lpsram_mr_write(para, 4, mr_or_default(para->dram_mr1, clk <= 204 ? 32 : 96), 0);
			lpsram_mr_write(para, 8, mr_or_default(para->dram_mr2, 67), 0);

			writel(readl(LPSRAM_MCTL_200c) | 0x82000000, LPSRAM_MCTL_200c);
			writel(readl(LPSRAM_MCTL_2100) | 0x100, LPSRAM_MCTL_2100);
			writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | 0x60000000,
			       LPSRAM_MCTL_2200);
			writel(readl(0x0205217c) | 1, 0x0205217c);
			writel(2000, LPSRAM_MCTL_2180);
		}
	} else if (type >= 5 && type <= 8) {
		writel((readl(0x0205308c) & 0xffff0000) | 0x00c000ff, 0x0205308c);
		writel(readl(LPSRAM_MCTL_2000) | 7, LPSRAM_MCTL_2000);
		writel((readl(LPSRAM_MCTL_2004) & ~0x300u) | (1u << 12), LPSRAM_MCTL_2004);
		writel((readl(LPSRAM_MCTL_200c) & 0x3ffffff) | 0x88000000, LPSRAM_MCTL_200c);
		writel(readl(LPSRAM_MCTL_200c) & ~(1u << 16), LPSRAM_MCTL_200c);
		writel(readl(LPSRAM_MCTL_2100) | 2, LPSRAM_MCTL_2100);
		writel(readl(LPSRAM_MCTL_2118) & 0xfffffff, LPSRAM_MCTL_2118);
		writel((readl(LPSRAM_MCTL_211c) & ~0xfu) | 8, LPSRAM_MCTL_211c);
		writel(readl(LPSRAM_MCTL_2124) & ~4u, LPSRAM_MCTL_2124);

		/*
		 * Body dispatch (original .L168): body A below runs for
		 * types 6, 7 and 8; the clock-table body B runs for type 5.
		 */
		if (type >= 6) {
			{
				uint32_t tpr1 = para->dram_tpr1;
				uint32_t lo = ((tpr1 >> 8) & 0x7f) ? (tpr1 & 0x7f00) : 0x500;
				uint32_t hi = ((tpr1 >> 16) & 0x7f) ? (tpr1 & 0x7f0000) : 0x60000;

				writel((readl(LPSRAM_MCTL_2138) & 0xff808080) | lo | hi | 0xc,
				       LPSRAM_MCTL_2138);
			}
			writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x1e002000,
			       LPSRAM_MCTL_2150);
			writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29),
			       LPSRAM_MCTL_2200);
			writel((readl(LPSRAM_MCTL_2204) & 0xffffff) | 0x13000000,
			       LPSRAM_MCTL_2204);
			writel((readl(0x02052214) & 0xffff8080) | 0x52c, 0x02052214);
			lpsram_mr_write(para, 1, mr_or_default(para->dram_mr1, 193),
					mr_hi_or_default(para->dram_mr1, 175));
			lpsram_mr_write(para, 0, mr_or_default(para->dram_mr0, 95),
					mr_hi_or_default(para->dram_mr0,
							 type == 8 ? 134 : 142));
			{
				uint32_t tpr0 = para->dram_tpr0;
				uint32_t val = ((tpr0 >> 8) & 0x7f) ? (tpr0 & 0x7f00) : 0x2c00;

				val |= readl(LPSRAM_MCTL_2150) & 0x80ff80ff;
				val |= ((tpr0 >> 24) & 0x7f) ? (tpr0 & 0x7f000000) : 0x2c000000;
				writel(val, LPSRAM_MCTL_2150);
			}
			writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 4, LPSRAM_MCTL_2130);
			writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | 0x60000000,
			       LPSRAM_MCTL_2200);
		} else {	/* type 5 */
			if (clk <= 166) {
				writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x19001c00,
				       LPSRAM_MCTL_2150);
				writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29),
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xffffff) | 0x13000000,
				       LPSRAM_MCTL_2204);
				writel((readl(0x02052214) & 0xffff8080) | 0x51c, 0x02052214);
				writel(readl(LPSRAM_MCTL_2130) & ~0xfu, LPSRAM_MCTL_2130);
			} else if (clk <= 233) {
				writel((readl(LPSRAM_MCTL_2150) & 0x80ff80ff) | 0x1c001c00,
				       LPSRAM_MCTL_2150);
				writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 29),
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xffffff) | 0x13000000,
				       LPSRAM_MCTL_2204);
				writel((readl(0x02052214) & 0xffff8080) | 0x51c, 0x02052214);
				writel((readl(LPSRAM_MCTL_2130) & ~0xfu) | 4, LPSRAM_MCTL_2130);
			}
			lpsram_mr_write(para, 0, mr_or_default(para->dram_mr0, 28),
					mr_hi_or_default(para->dram_mr0, 143));
			writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | 0x60000000,
			       LPSRAM_MCTL_2200);
		}
	}

	lpsram_controller_initial_finish(para);
}

/* ------------------------------------------------------------------------ */
/* Controller (re)configuration: bus width and refresh                        */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_controller_cfg(lpsram_para_t *para)
{
	uint32_t width_flag = para->dram_para2 & 0x11;
	uint32_t type = para->dram_type;
	uint32_t clk = para->dram_clk;
	uint32_t v_or;

	switch (width_flag) {
	case 0:
		v_or = 0x80000000;
		break;
	case 16:
		v_or = 0xc0000000;
		break;
	case 17:
		v_or = 0xe0000000;
		break;
	default:
		printf_dram("[Error] Not supported data width configuration !\n");
		return 0;
	}

	writel((readl(LPSRAM_MCTL_200c) & 0x1fffffff) | v_or, LPSRAM_MCTL_200c);

	if (width_flag == 17) {
		writel((readl(0x0205308c) & 0xffff5f00) | 0xa029, 0x0205308c);
		if (type == 3)
			lpsram_mr_write(para, 8, 3, 0);
		writel((readl(0x02053094) & ~3u) | 1, 0x02053094);
	}

	/*
	 * The original runs this refresh-timing ladder ONLY for type 4
	 * (APS3208E); every other type keeps the controller timings set up by
	 * lpsram_controller_initial_cfg.
	 */
	if (type == 4) {
		if (clk <= 204) {
			writel((readl(LPSRAM_MCTL_2200) & 0xffffff) | (1u << 30),
			       LPSRAM_MCTL_2200);
			switch (width_flag) {
			case 0:
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0x121200,
				       LPSRAM_MCTL_2204);
				break;
			case 16:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x81400,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0xa1200,
				       LPSRAM_MCTL_2204);
				break;
			case 17:
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0x101000,
				       LPSRAM_MCTL_2204);
				break;
			default:
				break;
			}
		} else if (clk <= 252) {
			writel(readl(LPSRAM_MCTL_2200) & 0xffffff, LPSRAM_MCTL_2200);
			switch (width_flag) {
			case 0:
			case 17:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x101c00,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0x121a00,
				       LPSRAM_MCTL_2204);
				break;
			case 16:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x81c00,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0xa1a00,
				       LPSRAM_MCTL_2204);
				break;
			default:
				break;
			}
		} else if (clk <= 408) {
			writel(readl(LPSRAM_MCTL_2200) & 0xffffff, LPSRAM_MCTL_2200);
			switch (width_flag) {
			case 0:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x40102a00,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0xa2700,
				       LPSRAM_MCTL_2204);
				break;
			case 16:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x82a00,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0xa2700,
				       LPSRAM_MCTL_2204);
				break;
			case 17:
				writel((readl(LPSRAM_MCTL_2200) & 0xff0000ff) | 0x103000,
				       LPSRAM_MCTL_2200);
				writel((readl(LPSRAM_MCTL_2204) & 0xff0000ff) | 0xa2700,
				       LPSRAM_MCTL_2204);
				break;
			default:
				break;
			}
		}
	}

	if (m_early_terminate_flag != 0) {
		writel((readl(LPSRAM_MCTL_2154) & 0x80ffffff) | (1u << 30),
		       LPSRAM_MCTL_2154);
		writel((readl(LPSRAM_MCTL_2124) & 0xffff80ff) | (1u << 14),
		       LPSRAM_MCTL_2124);
	}
	return 1;
}

void lpsram_phy_cfg(void)
{
	writel(readl(LPSRAM_MAST_2014) | 0x80000000, LPSRAM_MAST_2014);
}

/* ------------------------------------------------------------------------ */
/* Per-dx UI delay compensation                                              */
/* ------------------------------------------------------------------------ */

/*
 * 6-argument form of the original: (para, dx, adj_lo, sub_lo, adj_hi,
 * sub_hi).  adj_* is doubled before use; sub_* selects subtract (nonzero)
 * vs add (zero) per field.  Only the dx-th DX block register
 * (0x03103308 + dx*128) is written; dx == 255 applies to all blocks (the
 * original's `beq a1,255` guard).  The hi field is 16 bits wide
 * ([23:8]) and mirrors into bits [23:16] on write.
 */
void lpsram_phy_dx_ui_delay_compensation(lpsram_para_t *para, uint32_t dx,
					 uint32_t adj_lo, uint32_t sub_lo,
					 uint32_t adj_hi, uint32_t sub_hi)
{
	uint32_t ndx = 2 - (para->dram_para2 & 1);
	uint32_t lo = adj_lo << 1;
	uint32_t hi = adj_hi << 1;
	uint32_t i;

	writel(readl(LPSRAM_PHY_3100) & ~(1u << 26), LPSRAM_PHY_3100);

	for (i = 0; i < ndx; i++) {
		uint32_t reg = LPSRAM_PHY_3308 + i * 128;
		uint32_t val;
		uint32_t cur_lo, cur_hi;
		uint32_t new_lo, new_hi;

		if (dx != 255 && i != dx)
			continue;

		val = readl(reg);
		cur_lo = val & 0xff;
		cur_hi = (val >> 8) & 0xffff;

		if (sub_lo == 0)
			new_lo = (cur_lo + lo > 255) ? 255 : cur_lo + lo;
		else
			new_lo = (lo >= cur_lo) ? 0 : cur_lo - lo;

		if (sub_hi == 0)
			new_hi = (cur_hi + hi > 255) ? 255 : cur_hi + hi;
		else
			new_hi = (hi >= cur_hi) ? 0 : cur_hi - hi;

		writel((val & 0xff000000) | new_lo | (new_hi << 8) | (new_hi << 16),
		       reg);
	}

	writel(readl(LPSRAM_PHY_3100) | 0x06000000, LPSRAM_PHY_3100);
}

/* ------------------------------------------------------------------------ */
/* Core init: sys clocks + ZQ + PHY + controller                              */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_core_init(lpsram_para_t *para)
{
	uint32_t msi_rc;
	uint32_t cfg_rc;
	uint32_t t;

	lpsram_sys_init_cfg(para);
	lpsram_vrefzq_init(para);
	msi_rc = msi_phy_initial_cfg(para);

	/*
	 * dram_tpr4 packs per-dx UI delay adjustments:
	 * dx0: [6:0] adj_lo, [7] sub_lo, [14:8] adj_hi, [15] sub_hi;
	 * dx1: [22:16] adj_lo, [23] sub_lo, [30:24] adj_hi, [31] sub_hi.
	 */
	t = para->dram_tpr4;
	if (t != 0) {
		lpsram_phy_dx_ui_delay_compensation(para, 0, t & 0x7f,
						    (t >> 7) & 1,
						    (t >> 8) & 0x7f,
						    (t >> 15) & 1);
		if ((para->dram_para2 & 1) == 0)
			lpsram_phy_dx_ui_delay_compensation(para, 1,
							    (t >> 16) & 0x7f,
							    (t >> 23) & 1,
							    (t >> 24) & 0x7f,
							    (t >> 31) & 1);
	}

	lpsram_controller_initial_cfg(para);
	cfg_rc = lpsram_controller_cfg(para);
	lpsram_phy_cfg();

	return cfg_rc & msi_rc & 1;
}

/* ------------------------------------------------------------------------ */
/* Simple write/read sanity test                                             */
/* ------------------------------------------------------------------------ */

int lpsram_simple_wr_test(uint32_t size_mb, uint32_t words)
{
	volatile uint32_t *lo = (volatile uint32_t *)(uintptr_t)PSRAM_MEM_BASE;
	volatile uint32_t *hi =
		(volatile uint32_t *)(uintptr_t)(PSRAM_MEM_BASE +
						 ((size_mb >> 1) << 20));
	uint32_t i;

	for (i = 0; i < words; i++) {
		lo[i] = 0x01234567 + i;
		hi[i] = 0xfedcba98 + i;
	}
	for (i = 0; i < words; i++) {
		/*
		 * Each word is read exactly once, like the original: the
		 * value printed on failure is the value that failed the
		 * comparison, not a second (later) read.
		 */
		uint32_t hv = hi[i];

		if (hv != 0xfedcba98 + i) {
			printf_dram("LPSRAM simple test FAIL.\n");
			printf_dram("%x != %x at address %x\n",
				    hv, 0xfedcba98 + i,
				    (uint32_t)(uintptr_t)&hi[i]);
			return 1;
		}
		{
			uint32_t lv = lo[i];

				if (lv != 0x01234567 + i) {
				printf_dram("LPSRAM simple test FAIL.\n");
				printf_dram("%x != %x at address %x\n",
					    lv, 0x01234567 + i,
					    (uint32_t)(uintptr_t)&lo[i]);
				return 1;
			}
		}
	}
	printf_dram("LPSRAM simple test OK.\n");
	return 0;
}

/* ------------------------------------------------------------------------ */
/* Parameter dump                                                            */
/* ------------------------------------------------------------------------ */

void print_dram_para(const lpsram_para_t *para)
{
	printf_dram("\n");
	printf_dram("dram_clk    = %d\n", para->dram_clk);
	printf_dram("dram_type   = 0x%x\n", para->dram_type);
	printf_dram("dram_zq     = 0x%x\n", para->dram_zq);
	printf_dram("dram_odt_en = 0x%x\n", para->dram_odt_en);
	printf_dram("dram_para1  = 0x%x\n", para->dram_para1);
	printf_dram("dram_para2  = 0x%x\n", para->dram_para2);
	printf_dram("dram_mr0    = 0x%x\n", para->dram_mr0);
	printf_dram("dram_mr1    = 0x%x\n", para->dram_mr1);
	printf_dram("dram_mr2    = 0x%x\n", para->dram_mr2);
	printf_dram("dram_mr3    = 0x%x\n", para->dram_mr3);
	printf_dram("dram_tpr0   = 0x%x\n", para->dram_tpr0);
	printf_dram("dram_tpr1   = 0x%x\n", para->dram_tpr1);
	printf_dram("dram_tpr2   = 0x%x\n", para->dram_tpr2);
	printf_dram("dram_tpr3   = 0x%x\n", para->dram_tpr3);
	printf_dram("dram_tpr4   = 0x%x\n", para->dram_tpr4);
	printf_dram("dram_tpr5   = 0x%x\n", para->dram_tpr5);
	printf_dram("dram_tpr6   = 0x%x\n", para->dram_tpr6);
	printf_dram("dram_tpr7   = 0x%x\n", para->dram_tpr7);
	printf_dram("dram_tpr8   = 0x%x\n", para->dram_tpr8);
	printf_dram("dram_tpr9   = 0x%x\n", para->dram_tpr9);
	printf_dram("dram_tpr10  = 0x%x\n", para->dram_tpr10);
	printf_dram("dram_tpr11  = 0x%x\n", para->dram_tpr11);
	printf_dram("dram_tpr12  = 0x%x\n", para->dram_tpr12);
	printf_dram("dram_tpr13  = 0x%x\n", para->dram_tpr13);
	printf_dram("\n");
}

/* ------------------------------------------------------------------------ */
/* Top-level entry                                                           */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_init(void *buff)
{
	lpsram_para_t *para = (lpsram_para_t *)buff;
	uint32_t type;
	uint32_t size = 0;
	uint32_t chipid;
	uint32_t rc;
	uint32_t v;

	chipid = sid_read_key(0) & 0xffff;

	if (chipid != 0 && (para->dram_tpr13 & 1) == 0) {
		type = lpsram_get_type_by_hwid();
		para->dram_type = type;
		if (type == 0)
			return 0;
		printf_dram("LPSRAM type detection done.\n");
		lpsram_para_merge_config(para);
	}

	if (para->dram_tpr13 & 0x20000000)
		print_dram_para(para);

	writel(readl(SYSCFG_REG_160) & ~2u, SYSCFG_REG_160);
	writel(0x19370505, SYSCFG_REG_174);
	printf_dram("ZQ value = 0x%x\n", readl(SYSCFG_REG_174));

	dram_vol_set(para);

	memset(para->rdq_delay, 0, sizeof(para->rdq_delay));
	memset(para->wdq_delay, 0, sizeof(para->wdq_delay));

	if ((int32_t)para->dram_tpr10 >= 0 && (para->dram_tpr10 & 0x00080000)) {
		dcache_enable();
		rc = dst_from_none(para);
		if (rc != 0)
			return 0;
		para->dram_tpr10 |= 0x80000000;
		dcache_disable();
	}

	rc = lpsram_core_init(para);
	if (rc == 0) {
		printf_dram("LPSRAM initial error: 1 !\n");
		return 0;
	}

	lpsram_enable_all_master();

	printf_dram("LPSRAM BOOT DRIVE INFO: %s\n", "V0.56");
	printf_dram("LPSRAM CLK = %d MHz\n", para->dram_clk);
	printf_dram("LPSRAM Type = %s\n", get_dram_type_name(para->dram_type));
	v = readl(LPSRAM_PHY_3140);
	printf_dram("LPSRAM ZQ PARAM = 0x%x\n", v & 0xffffff);

	size = para->dram_para1 & 0xffff;
	printf_dram("LPSRAM SIZE = %d MB\n", size);

	if (lpsram_simple_wr_test(size, 0x1000) != 0) {
		printf_dram("[ERROR] LSPSRAM INIT failed.\n");
		return 0;
	}

	if (para->dram_tpr13 & 0x20000000)
		print_dram_para(para);

	return size;
}

/* ------------------------------------------------------------------------ */
/* PRNG used by the memory tester                                            */
/* ------------------------------------------------------------------------ */

uint32_t rand(void)
{
	Seed = Seed * 0x015a4e35 + 1;
	return (Seed >> 16) & 0x7fff;
}

/* ------------------------------------------------------------------------ */
/* Memory-tester primitives                                                  */
/* ------------------------------------------------------------------------ */

int dst_compare_regions(volatile uint32_t *a, volatile uint32_t *b, uint32_t len)
{
	uint32_t i;

	FlushDcacheAll();
	for (i = 0; i < len; i++) {
		if (a[i] != b[i])
			return -1;
	}
	return 0;
}

int dst_test_stuck_address(volatile uint32_t *base, uint32_t len)
{
	uint32_t pass;
	uint32_t i;

	for (pass = 0; pass < 16; pass++) {
		volatile uint32_t *p;

		for (i = 0, p = base; i < len; i++, p++)
			*p = ((i + pass) & 1) ? ~(uint32_t)(uintptr_t)p
					      : (uint32_t)(uintptr_t)p;
		FlushDcacheAll();
		for (i = 0, p = base; i < len; i++, p++) {
			uint32_t expect = ((i + pass) & 1) ? ~(uint32_t)(uintptr_t)p
							   : (uint32_t)(uintptr_t)p;
			uint32_t got = *p;

			if (got != expect) {
				uint32_t x = (uint32_t)(uintptr_t)p ^ got;

				return ((i + pass) & 1) ? ~x : x;
			}
		}
	}
	return 0;
}

int dst_test_bitflip_comparison(volatile uint32_t *a, volatile uint32_t *b,
				uint32_t len)
{
	uint32_t bit;
	uint32_t round;
	uint32_t i;

	for (bit = 0; bit < 16; bit++) {
		uint32_t pat = 1u << bit;
		uint32_t npat = ~pat;

		for (round = 0; round < 8; round++) {
			for (i = 0; i < len; i++) {
				uint32_t v = (i & 1) ? pat : npat;

				b[i] = v;
				a[i] = v;
			}
			if (dst_compare_regions(a, b, len) != 0)
				return -1;
		}
	}
	return 0;
}

int dst_dramc_memtester_test(uint32_t base1, uint32_t base2, uint32_t len)
{
	int rc;

	rc = dst_test_stuck_address((volatile uint32_t *)(uintptr_t)base1, len);
	if (rc != 0)
		return rc;
	return dst_test_bitflip_comparison((volatile uint32_t *)(uintptr_t)base1,
					   (volatile uint32_t *)(uintptr_t)base2,
					   len);
}

void dst_dramc_write(volatile uint32_t *addr, uint32_t len, uint32_t data)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		addr[i] = data;
}

void dst_dramc_write_2data(volatile uint32_t *addr, uint32_t len,
			   uint32_t data0, uint32_t data1)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		addr[i] = (i & 1) ? data1 : data0;
}

int dst_dramc_memtester_bit_test(uint32_t base_w, uint32_t base_c, uint32_t len,
				 uint32_t bits)
{
	/* Pattern table from the original .rodata: 6 static words followed by
	 * walking-bit patterns for bits [0, len). */
	static const uint32_t patterns[6] = {
		0xffffffff, 0xffff0000, 0xff00ff00, 0x0f0f0f0f, 0x5a5a5a5a,
		0x55555555,
	};
	volatile uint32_t *w = (volatile uint32_t *)(uintptr_t)base_w;
	volatile uint32_t *r = (volatile uint32_t *)(uintptr_t)0x40100000;
	volatile uint32_t *c = (volatile uint32_t *)(uintptr_t)base_c;
	uint32_t i;
	uint32_t bit;

	for (i = 0; i < 6; i++) {
		dst_dramc_write(w, len, patterns[i]);
		dst_dramc_write(r, len, patterns[i]);
		memcpy((void *)(uintptr_t)base_c, (const void *)(uintptr_t)0x40100000,
		       len * 4);
		if (dst_compare_regions(c, w, len) != 0)
			return -1;
	}

	if (len >= 8)
		len &= ~7u;
	for (bit = 0; bit < bits && bit < 32; bit++) {
		uint32_t on = 1u << bit;
		uint32_t inv;

		for (inv = 0; inv < 2; inv++) {
			dst_dramc_write_2data(w, len, inv ? ~on : on, inv ? on : ~on);
			dst_dramc_write_2data(r, len, inv ? ~on : on, inv ? on : ~on);
			memcpy((void *)(uintptr_t)base_c,
			       (const void *)(uintptr_t)0x40100000, len * 4);
			if (dst_compare_regions(c, w, len) != 0)
				return -1;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------------ */
/* UI delay training: single test point                                      */
/* ------------------------------------------------------------------------ */

int lpsram_ui_dly_test_point(lpsram_para_t *para, uint32_t mode, int32_t neg,
			     int32_t pos)
{
	uint32_t aneg = (neg < 0) ? -neg : neg;
	uint32_t apos = (pos < 0) ? -pos : pos;
	uint32_t sneg = (neg < 0) ? 1u : 0u;
	uint32_t spos = (pos < 0) ? 1u : 0u;
	uint32_t tpr10 = para->dram_tpr10;
	uint32_t val;
	uint32_t rc;
	uint32_t base;

	aneg &= 0x7f;
	apos &= 0x7f;

	if (tpr10 & 0x7f00) {
		uint32_t sh = mode * 16;
		uint32_t mask = 0xffff << sh;

		val = para->dram_tpr4 & ~mask;
		val |= (uint32_t)aneg << sh;
		val |= (uint32_t)apos << (sh + 8);
		val |= sneg << (sh + 7);
		val |= spos << (sh + 15);
	} else {
		val = (uint32_t)aneg;
		val |= (uint32_t)apos << 8;
		val |= sneg << 7;
		val |= spos << 15;
		val |= val << 16;
	}
	writel(val, (uintptr_t)&para->dram_tpr4);

	rc = lpsram_core_init(para);
	if (rc == 0)
		return 1;

	base = PSRAM_MEM_BASE + (((para->dram_para2 >> 1) & 0x7fff) << 20);
	rc = dst_dramc_memtester_test(PSRAM_MEM_BASE, base, 0x800);
	FlushDcacheAll();
	return rc;
}

/* ------------------------------------------------------------------------ */
/* UI delay window scan                                                      */
/* ------------------------------------------------------------------------ */

static uint32_t ui_dly_scan_limit(lpsram_para_t *para, uint32_t mode,
				  uint32_t mode2)
{
	uint32_t dxn = (para->dram_para2 & 1) ? 1 : 2;
	uint32_t limit;

	if (mode == 0 || mode == 255) {
		limit = (readl(LPSRAM_PHY_3308) >> 8) & 0xff;
		limit >>= 1;
		if (dxn == 2 && (mode == 255 || mode == 1)) {
			uint32_t w = readl(LPSRAM_PHY_3388);
			uint32_t lim = (mode2 == 0) ? (w & 0xff) : ((w >> 8) & 0xff);

			if ((lim >> 1) < limit)
				limit = lim >> 1;
		}
	} else {
		limit = 127;
		if (dxn == 2 && (mode == 255 || mode == 1)) {
			uint32_t w = readl(LPSRAM_PHY_3388);
			uint32_t lim = (mode2 == 0) ? (w & 0xff) : ((w >> 8) & 0xff);

			if ((lim >> 1) < limit)
				limit = lim >> 1;
		}
	}
	return limit;
}

int lpsram_ui_dly_scan_window(lpsram_para_t *para, uint32_t mode, uint32_t mode2,
			      uint32_t fixed, int32_t *out_start, int32_t *out_end)
{
	const char *dir = (mode == 1) ? "0" : "1";
	uint32_t limit;
	int32_t best_start = 0;
	int32_t best_len = 0;
	int32_t pt;

	if (lpsram_ui_dly_test_point(para, mode,
				     mode2 ? 0 : (int32_t)mode2,
				     mode2 ? 0 : (int32_t)fixed) != 0)
		printf_dram("[UI DLY TRAIN] lcdl default value fail!\n");

	limit = ui_dly_scan_limit(para, mode, mode2);
	printf_dram("[UI DLY TRAIN] saturation limit: neg=%d pos=%d\n",
		    limit, limit);
	if (mode2)
		printf_dram("[UI DLY TRAIN]==== WDQ%s scan (rdqs=%d) ====\n",
			    dir, mode2);
	else
		printf_dram("[UI DLY TRAIN]==== RDQS%s scan (wdq=%d) ====\n",
			    dir, mode2);

	/* Negative direction: walk down from -1 until two consecutive failures. */
	{
		int32_t run_start = 0, run_len = 0;
		uint32_t confirm = 0;

		for (pt = -1; pt >= -(int32_t)limit; pt--) {
			int ok = lpsram_ui_dly_test_point(para, mode,
							  mode2 ? 0 : pt,
							  mode2 ? pt : (int32_t)fixed);

			if (ok == 0) {
				if (pt == run_start - 1) {
					run_len++;
				} else {
					if (best_len < run_len) {
						best_len = run_len;
						best_start = run_start;
					}
					run_start = pt;
					run_len = 1;
				}
				confirm = 0;
			} else {
				if (confirm == 0 && best_len < run_len) {
					best_len = run_len;
					best_start = run_start;
				}
				if (++confirm >= 2)
					break;
			}
		}
		if (best_len < run_len) {
			best_len = run_len;
			best_start = run_start;
		}
	}

	/* Positive direction: walk up from 1 until two consecutive failures. */
	{
		int32_t run_start = 1, run_len = 0;
		uint32_t confirm = 0;

		for (pt = 1; pt <= (int32_t)limit; pt++) {
			int ok = lpsram_ui_dly_test_point(para, mode,
							  mode2 ? 0 : pt,
							  mode2 ? pt : (int32_t)fixed);

			if (ok == 0) {
				if (pt == run_start + run_len) {
					run_len++;
				} else {
					if (best_len < run_len) {
						best_len = run_len;
						best_start = run_start;
					}
					run_start = pt;
					run_len = 1;
				}
				confirm = 0;
			} else {
				if (confirm == 0 && best_len < run_len) {
					best_len = run_len;
					best_start = run_start;
				}
				if (++confirm >= 2)
					break;
			}
		}
		if (best_len < run_len) {
			best_len = run_len;
			best_start = run_start;
		}
	}

	*out_start = best_start;
	*out_end = best_start + best_len - 1;
	if (mode2)
		printf_dram("[UI DLY TRAIN] WDQ%s window [%d, %d] len=%d center=%d\n",
			    dir, *out_start, *out_end, best_len,
			    (*out_start + *out_end) / 2);
	else
		printf_dram("[UI DLY TRAIN] RDQS%s window [%d, %d] len=%d center=%d\n",
			    dir, *out_start, *out_end, best_len,
			    (*out_start + *out_end) / 2);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* UI delay bit training (soft training phase 1)                             */
/* ------------------------------------------------------------------------ */

uint32_t lpsram_phy_dx_ui_delay_training(lpsram_para_t *para)
{
	int32_t neg = 0, pos = 0, neg2 = 0, pos2 = 0;
	uint32_t center, center2;
	uint32_t val;

	writel(0, (uintptr_t)&para->dram_tpr4);

	if ((para->dram_tpr10 & 0x02000000) == 0) {
		/* Single-dx path: one shared window pair. */
		if (lpsram_ui_dly_scan_window(para, 255, 1, 0, &neg, &pos) != 0)
			return 2;
		center = ((uint32_t)(neg + pos)) / 2;
		if (lpsram_ui_dly_scan_window(para, 255, 0, center, &neg2, &pos2) != 0)
			return 3;
		center2 = ((uint32_t)(neg2 + pos2)) / 2;
		if (lpsram_ui_dly_test_point(para, 255, center2, center) != 0) {
			printf_dram("[UI DLY TRAIN] ALL center verify fail\n");
			return 4;
		}
		val = (center2 & 0x7f) | ((center2 >> 31 ? 1u : 0u) << 7) |
		      ((center & 0x7f) << 8) | ((center >> 31 ? 1u : 0u) << 15);
		writel(val | (val << 16), (uintptr_t)&para->dram_tpr4);
		printf_dram("[UI DLY TRAIN] center dram_tpr4 = 0x%08x, verify pass\n",
			    val | (val << 16));
		return 0;
	}

	/* Dual-dx path: independent window per byte lane. */
	{
		uint32_t lanes = (para->dram_para2 & 1) ? 1 : 2;
		uint32_t lane;
		uint32_t acc = 0;

		for (lane = 0; lane < lanes; lane++) {
			if (lpsram_ui_dly_scan_window(para, lane, 1, 0, &neg, &pos) != 0)
				return 2;
			center = ((uint32_t)(neg + pos)) / 2;
			if (lpsram_ui_dly_scan_window(para, lane, 0, center, &neg2, &pos2) != 0)
				return 3;
			center2 = ((uint32_t)(neg2 + pos2)) / 2;
			if (lpsram_ui_dly_test_point(para, lane, center2, center) != 0) {
				printf_dram("[UI DLY TRAIN] dbyte%d center verify fail\n",
					    lane);
				return 4;
			}
			val = (center2 & 0x7f) | ((center2 >> 31 ? 1u : 0u) << 7) |
			      ((center & 0x7f) << 8) | ((center >> 31 ? 1u : 0u) << 15);
			acc |= val << (lane * 16);
			writel(acc, (uintptr_t)&para->dram_tpr4);
		}
		printf_dram("[UI DLY TRAIN] center dram_tpr4 = 0x%08x, verify pass\n",
			    acc);
	}
	return 0;
}

/* ------------------------------------------------------------------------ */
/* DQ eye scan (per-bit RDQ/WDQ window refinement)                           */
/* ------------------------------------------------------------------------ */

uint32_t dst_dq_eye_scan(lpsram_para_t *para)
{
	uint32_t rdq_win[8];
	uint32_t wdq_win[8];
	uint32_t rc;
	uint32_t clk = para->dram_clk;
	uint32_t loops;
	uint32_t t;

	/* Mirror the per-bit trained windows into local arrays. */
	memcpy(rdq_win, &para->dram_tpr11, sizeof(rdq_win));
	memset(wdq_win, 0, sizeof(wdq_win));
	(void)wdq_win;

	loops = (((para->dram_tpr10 >> 16) & 7) * 40000);
	loops /= 500000000u / (((readl(LPSRAM_PHY_3300) >> 8) & 0xff) * clk);
	{
		uint32_t loops2 = ((readl(LPSRAM_PHY_3388) >> 8) & 0xff) * clk;

		loops2 = 500000000u / loops2;
		(void)loops2;
	}

	if (lpsram_core_init(para) == 0) {
		printf_dram("[ERROR SOFT TRAINING] Lpsram Soft Training fail\n");
		for (;;)
			;
	}

	rc = dst_dramc_memtester_test(PSRAM_MEM_BASE, 0x40400000, 0x100000);
	FlushDcacheAll();
	if (rc != 0) {
		printf_dram("[ERROR SOFT TRAINING] Stable memtest fail\n");
		return 1;
	}
	printf_dram("[SOFT TRAINING] CLK=%dM Stable memtest pass\n", clk);

	/* Refine each 4-bit RDQ/WDQ field by walking the delay line and testing
	 * the walking-bit patterns; keep the centre of the passing window. */
	for (t = 0; t < 16; t++) {
		uint32_t shift = (t % 4) * 4;
		uint32_t mask = 0xfu << shift;
		uint32_t *win = (t < 8) ? rdq_win : wdq_win;
		uint32_t idx = t % 8;
		int32_t start = (win[idx] >> shift) & 0xf;
		int32_t run = 0, best = 0, best_at = 0, d;

		for (d = 0; d <= 15; d++) {
			uint32_t delay = ((start + d) & 0xf);
			uint32_t old = readl(LPSRAM_PHY_3100);

			writel((old & ~mask) | (delay << shift), LPSRAM_PHY_3100);
			rc = dst_dramc_memtester_bit_test(0x40200000, 0x40300000,
							  0x100, 4);
			FlushDcacheAll();
			if (rc == 0) {
				if (run == 0)
					best_at = d;
				run++;
				if (run > best)
					best = run;
			} else {
				run = 0;
			}
		}
		if (best != 0)
			win[idx] = (win[idx] & ~mask) |
				   (((best_at + best / 2) & 0xf) << shift);
	}

	para->dram_tpr11 = 0;
	para->dram_tpr12 = 0;
	memcpy(&para->dram_tpr11, rdq_win, sizeof(rdq_win));

	if (lpsram_core_init(para) != 0) {
		printf_dram("[ERROR SOFT TRAINING] Stable memtest fail\n");
		return 1;
	}
	rc = dst_dramc_memtester_test(PSRAM_MEM_BASE, 0x40400000, 0x100000);
	FlushDcacheAll();
	printf_dram("[SOFT TRAINING] Version: %s\n", "T2.5");
	if (rc != 0) {
		printf_dram("[ERROR SOFT TRAINING] Stable test, dram_clk=%d, "
			    "dram_tpr11=0x%08x, dram_tpr12=0x%08x, memtest fail\n",
			    para->dram_clk, para->dram_tpr11, para->dram_tpr12);
		return 1;
	}
	printf_dram("[SOFT TRAINING] Stable test, dram_clk=%d, "
		    "dram_tpr11=0x%08x, dram_tpr12=0x%08x, memtest pass\n",
		    para->dram_clk, para->dram_tpr11, para->dram_tpr12);
	return 0;
}

/* ------------------------------------------------------------------------ */
/* Soft-training dispatcher (enabled by dram_tpr10 bits 24/26)                */
/* ------------------------------------------------------------------------ */

uint32_t dst_from_none(lpsram_para_t *para)
{
	uint32_t rc = 0;
	uint32_t loop;

	printf_dram("[SOFT TRAINING] Version: %s\n", "T2.5");

	for (loop = 1; loop <= 5; loop++) {
		printf_dram("[SOFT TRAINING] Lpsram Soft Training Loop%d\n", loop);

		if (para->dram_tpr10 & 0x01000000) {
			rc = lpsram_phy_dx_ui_delay_training(para);
			if (rc != 0) {
				printf_dram("[ERROR SOFT TRAINING] Lpsram Soft Training fail\n");
				return 1;
			}
		}

		if ((para->dram_tpr10 & 0x04000000) == 0) {
			rc = dst_dq_eye_scan(para);
			if (rc != 0)
				continue;
		}

		rc = lpsram_core_init(para);
		rc = (rc == 0) ? 1 : 0;
		rc |= dst_dramc_memtester_test(PSRAM_MEM_BASE, 0x40400000, 0x100000);
		if (rc != 0) {
			printf_dram("[ERROR SOFT TRAINING] Stable test, dram_clk=%d, "
				    "dram_tpr11=0x%08x, dram_tpr12=0x%08x, memtest fail\n",
				    para->dram_clk, para->dram_tpr11, para->dram_tpr12);
			continue;
		}
		printf_dram("[SOFT TRAINING] Stable test, dram_clk=%d, "
			    "dram_tpr11=0x%08x, dram_tpr12=0x%08x, memtest pass\n",
			    para->dram_clk, para->dram_tpr11, para->dram_tpr12);
		return 0;
	}

	printf_dram("[ERROR SOFT TRAINING] Lpsram Soft Training fail\n");
	return 1;
}

/**
 * @brief Microsecond delay helper used by the PSRAM initialization code.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay(us);
}

/**
 * @brief Invalidate the entire data cache.
 */
void csi_l2c_clear_invalid_all(void)
{
	invalidate_dcache_all();
	return;
}

/**
 * @brief Clean (flush) the entire data cache.
 */
void csi_l2c_clear_all(void)
{
	flush_dcache_all();
	return;
}

/**
 * @brief Set the DDR voltage.
 *
 * This platform has no software-controlled DDR supply, so the request is
 * always accepted and reports success without programming any regulator.
 *
 * @param[in] vol_val Requested voltage value in millivolts.
 * @return 0 on success.
 */
int set_ddr_voltage(unsigned int vol_val)
{
	return 0;
}

/**
 * @brief Initialize the PSRAM controller and memory.
 *
 * Initializes PSRAM with the parameters supplied in the descriptor and records
 * the reported memory size.
 *
 * @param[in,out] psram PSRAM descriptor carrying the parameter table.
 * @return The initialized PSRAM size in megabytes, or zero on failure.
 */
uint32_t sunxi_psram_init(sunxi_psram_t *psram)
{
	if (psram == NULL || psram->parameter_count == 0U)
		return 0U;
	psram->size = lpsram_init((void *)psram->parameters);
	return psram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-psram");
