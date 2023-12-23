/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN8IW20_SYS_DRAM_H__
#define __SUN8IW20_SYS_DRAM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-dram.h"

#define SDRAM_BASE (0x40000000)

enum sunxi_dram_type {
    SUNXI_DRAM_TYPE_DDR2 = 2,
    SUNXI_DRAM_TYPE_DDR3 = 3,
    SUNXI_DRAM_TYPE_LPDDR2 = 6,
    SUNXI_DRAM_TYPE_LPDDR3 = 7,
};

typedef struct __DRAM_PARA {
    // normal configuration
    uint32_t dram_clk;
    uint32_t dram_type;// dram_type			DDR2: 2				DDR3: 3		LPDDR2: 6	LPDDR3: 7	DDR3L: 31
    uint32_t dram_zq;  // do not need
    uint32_t dram_odt_en;

    // control configuration
    uint32_t dram_para1;
    uint32_t dram_para2;

    // timing configuration
    uint32_t dram_mr0;
    uint32_t dram_mr1;
    uint32_t dram_mr2;
    uint32_t dram_mr3;
    uint32_t dram_tpr0;// DRAMTMG0
    uint32_t dram_tpr1;// DRAMTMG1
    uint32_t dram_tpr2;// DRAMTMG2
    uint32_t dram_tpr3;// DRAMTMG3
    uint32_t dram_tpr4;// DRAMTMG4
    uint32_t dram_tpr5;// DRAMTMG5
    uint32_t dram_tpr6;// DRAMTMG8
    // reserved for future use
    uint32_t dram_tpr7;
    uint32_t dram_tpr8;
    uint32_t dram_tpr9;
    uint32_t dram_tpr10;
    uint32_t dram_tpr11;
    uint32_t dram_tpr12;
    uint32_t dram_tpr13;
} dram_para_t;

int init_DRAM(int type, dram_para_t *para);

uint64_t sunxi_dram_init(void);

#endif // __SUN8IW20_SYS_DRAM_H__
