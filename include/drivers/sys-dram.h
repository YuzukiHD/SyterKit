/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __dram_head_h__
#define __dram_head_h__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "log.h"

#define SDRAM_BASE (0x40000000)

enum sunxi_dram_type {
	SUNXI_DRAM_TYPE_DDR2   = 2,
	SUNXI_DRAM_TYPE_DDR3   = 3,
	SUNXI_DRAM_TYPE_LPDDR2 = 6,
	SUNXI_DRAM_TYPE_LPDDR3 = 7,
};

typedef struct __DRAM_PARA {
	// normal configuration
	unsigned int dram_clk;
	unsigned int dram_type; // dram_type			DDR2: 2				DDR3: 3		LPDDR2: 6	LPDDR3: 7	DDR3L: 31
	unsigned int dram_zq; // do not need
	unsigned int dram_odt_en;

	// control configuration
	unsigned int dram_para1;
	unsigned int dram_para2;

	// timing configuration
	unsigned int dram_mr0;
	unsigned int dram_mr1;
	unsigned int dram_mr2;
	unsigned int dram_mr3;
	unsigned int dram_tpr0; // DRAMTMG0
	unsigned int dram_tpr1; // DRAMTMG1
	unsigned int dram_tpr2; // DRAMTMG2
	unsigned int dram_tpr3; // DRAMTMG3
	unsigned int dram_tpr4; // DRAMTMG4
	unsigned int dram_tpr5; // DRAMTMG5
	unsigned int dram_tpr6; // DRAMTMG8
	// reserved for future use
	unsigned int dram_tpr7;
	unsigned int dram_tpr8;
	unsigned int dram_tpr9;
	unsigned int dram_tpr10;
	unsigned int dram_tpr11;
	unsigned int dram_tpr12;
	unsigned int dram_tpr13;
} dram_para_t;

int init_DRAM(int type, dram_para_t *para);

unsigned long sunxi_dram_init(void);

#endif
