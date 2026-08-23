/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_DRAM_H__
#define __DRIVERS_DRAM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <drivers/pmu/axp.h>
#include <drivers/rtc/rtc.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

enum sunxi_dram_type {
	SUNXI_DRAM_TYPE_DDR2 = 2,
	SUNXI_DRAM_TYPE_DDR3 = 3,
	SUNXI_DRAM_TYPE_LPDDR2 = 6,
	SUNXI_DRAM_TYPE_LPDDR3 = 7,
};

#define SUNXI_DRAM_MAX_PARAM_WORDS 128U

typedef struct {
	uintptr_t base;
	size_t size;
} sunxi_dram_register_t;

typedef struct {
	sunxi_dram_register_t ccu;
	sunxi_dram_register_t aon_ccu;
	sunxi_dram_register_t mctl_com;
	sunxi_dram_register_t mctl_phy;
	sunxi_dram_register_t sysctrl;
	sunxi_dram_register_t sid;
	sunxi_dram_register_t r_cpucfg;
	sunxi_dram_register_t r_prcm;
	sunxi_dram_register_t pmu_rtc;
} sunxi_dram_registers_t;

typedef struct {
	uint32_t parameters[SUNXI_DRAM_MAX_PARAM_WORDS];
	size_t parameter_count;
	int dt_node;
	uint32_t size;
	uintptr_t memory_base; /**< CPU-visible DRAM base used during training. */
	size_t memory_size; /**< Size of the CPU-visible DRAM address window. */
	sunxi_dram_registers_t registers;
	uintptr_t init_code_base; /**< Reserved SRAM for external init code. */
	size_t init_code_size; /**< Capacity of the external init code region. */
	/* Optional board-supplied PMU handles; DT never resolves these. */
	axp_pmu_t *pmu;
	axp_pmu_t *pmu_aux;
	sunxi_rtc_t rtc;
} sunxi_dram_t;

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
 * @param dram DRAM instance to query.
 * @return The size of the DRAM in bytes.
 */
uint32_t sunxi_get_dram_size(const sunxi_dram_t *dram);

/**
 * @brief Initialize the DRAM.
 * 
 * This function initializes the DRAM with the specified parameters. The 
 * initialization process may involve configuration of memory controllers 
 * and other hardware settings. The user must provide a pointer to a 
 * structure containing the necessary initialization parameters.
 * 
 * @param dram DRAM instance containing the mutable initialization parameters.
 * @return A status code indicating the result of the initialization. 
 *         Typically returns zero on success and a non-zero value on 
 *         failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __DRIVERS_DRAM_H__
