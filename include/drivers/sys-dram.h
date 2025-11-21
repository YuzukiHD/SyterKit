/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_DRAM_H__
#define __SYS_DRAM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <reg/reg-dram.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/* Common is 0x40000000 */
#ifndef SDRAM_BASE
#define SDRAM_BASE (0x40000000)
#endif

enum sunxi_dram_type {
	SUNXI_DRAM_TYPE_DDR2 = 2,
	SUNXI_DRAM_TYPE_DDR3 = 3,
	SUNXI_DRAM_TYPE_LPDDR2 = 6,
	SUNXI_DRAM_TYPE_LPDDR3 = 7,
};

#if defined SUNXI_DRAM_PARAM_V2
typedef struct {
	uint32_t dram_clk;
	uint32_t dram_type;
	uint32_t dram_dx_odt;
	uint32_t dram_dx_dri;
	uint32_t dram_ca_dri;
	uint32_t dram_para0;
	uint32_t dram_para1;
	uint32_t dram_para2;
	uint32_t dram_mr0;
	uint32_t dram_mr1;
	uint32_t dram_mr2;
	uint32_t dram_mr3;
	uint32_t dram_mr4;
	uint32_t dram_mr5;
	uint32_t dram_mr6;
	uint32_t dram_mr11;
	uint32_t dram_mr12;
	uint32_t dram_mr13;
	uint32_t dram_mr14;
	uint32_t dram_mr16;
	uint32_t dram_mr17;
	uint32_t dram_mr22;
	uint32_t dram_tpr0;
	uint32_t dram_tpr1;
	uint32_t dram_tpr2;
	uint32_t dram_tpr3;
	uint32_t dram_tpr6;
	uint32_t dram_tpr10;
	uint32_t dram_tpr11;
	uint32_t dram_tpr12;
	uint32_t dram_tpr13;
	uint32_t dram_tpr14;
} dram_para_t;
#else
typedef struct {
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
#endif

/**
 * @brief Get the size of the DRAM (Dynamic Random Access Memory).
 * 
 * This function retrieves the total size of the DRAM available in the 
 * system. The size is returned in bytes.
 * 
 * @return The size of the DRAM in bytes.
 */
uint32_t sunxi_get_dram_size();

/**
 * @brief Initialize the DRAM.
 * 
 * This function initializes the DRAM with the specified parameters. The 
 * initialization process may involve configuration of memory controllers 
 * and other hardware settings. The user must provide a pointer to a 
 * structure containing the necessary initialization parameters.
 * 
 * @param para A pointer to a structure containing the parameters needed for 
 *              the initialization process.
 * @return A status code indicating the result of the initialization. 
 *         Typically returns zero on success and a non-zero value on 
 *         failure.
 */
uint32_t sunxi_dram_init(void *para);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_DRAM_H__
