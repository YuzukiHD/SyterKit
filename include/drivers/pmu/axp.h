/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __G_AXP_H__
#define __G_AXP_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <drivers/i2c/i2c.h>

#include "reg-axp.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * @brief Structure describing a voltage step of the power domain.
 */
typedef struct _axp_step_info {
	uint32_t step_min_vol;// Minimum voltage level for the step.
	uint32_t step_max_vol;// Maximum voltage level for the step.
	uint32_t step_val;	  // Voltage increment value for the step.
	uint32_t regation;	  // Regulator register address.
} axp_step_info_t;

/**
 * @brief Structure describing the control information of a power domain.
 */
typedef struct _axp_contrl_info {
	char name[8];					// Name of the power domain.
	uint32_t min_vol;				// Minimum voltage level for the domain.
	uint32_t max_vol;				// Maximum voltage level for the domain.
	uint32_t cfg_reg_addr;			// Configuration register address.
	uint32_t cfg_reg_mask;			// Configuration register mask.
	uint32_t ctrl_reg_addr;			// Control register address.
	uint32_t ctrl_bit_ofs;			// Bit offset in the control register.
	uint32_t reg_addr_offset;		// Offset of the register address.
	axp_step_info_t axp_step_tbl[4];// Voltage step table for the domain.
} axp_contrl_info;

typedef enum {
	AXP_PMU_AXP1530,
	AXP_PMU_AXP2101,
	AXP_PMU_AXP2202,
	AXP_PMU_AXP333,
	AXP_PMU_AXP8191,
} axp_pmu_type_t;

typedef struct axp_pmu {
	sunxi_i2c_t *i2c;
	uint8_t address;
	uint8_t fallback_address;
	axp_pmu_type_t type;
} axp_pmu_t;

static inline bool axp_pmu_matches(const axp_pmu_t *pmu,
				   axp_pmu_type_t type) {
	return pmu != NULL && pmu->i2c != NULL && pmu->address != 0U &&
	       pmu->type == type;
}

/* Common function */

/**
 * @brief Set the voltage for a specific power domain controlled by AXP.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @param set_vol Voltage value to set.
 * @param onoff Whether to turn on or off the power domain (1 for on, 0 for off).
 * @param axp_ctrl_tbl Pointer to the AXP control information table.
 * @param axp_ctrl_tbl_size Size of the AXP control information table.
 * @param axp_addr AXP device address.
 * @return Integer indicating the success status of the operation.
 */
int axp_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff,
		axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size);

/**
 * @brief Get the voltage value for a specific power domain controlled by AXP.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @param axp_ctrl_tbl Pointer to the AXP control information table.
 * @param axp_ctrl_tbl_size Size of the AXP control information table.
 * @param axp_addr AXP device address.
 * @return The voltage value of the specified power domain.
 */
int axp_get_vol(axp_pmu_t *pmu, char *name,
		axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size);

/* define AXP pmu */

/**
 * @brief Macro to define PMU functions for a specific AXP PMU chip.
 *
 * This macro generates a set of function prototypes for controlling and
 * interacting with a specified AXP Power Management Unit (PMU) chip.
 * 
 * The generated functions are:
 * - `pmu_<name>_init`: Initializes the PMU for the specified device.
 * - `pmu_<name>_get_vol`: Retrieves the voltage of a specified power rail.
 * - `pmu_<name>_set_vol`: Sets the voltage of a specified power rail.
 * - `pmu_<name>_dump`: Dumps the internal registers and status of the PMU.
 *
 * Example usage:
 * ```c
 * DEFINE_AXP_PMU(axp2202);
 * ```
 * This will generate the following function prototypes:
 * - `int pmu_axp2202_init(sunxi_i2c_t *i2c_dev);`
 * - `int pmu_axp2202_get_vol(sunxi_i2c_t *i2c_dev, char *name);`
 * - `int pmu_axp2202_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff);`
 * - `void pmu_axp2202_dump(sunxi_i2c_t *i2c_dev);`
 * 
 * @param name The name of the PMU chip (e.g., axp2202, axp221, etc.).
 */
#define DEFINE_AXP_PMU(name)                                                            \
	int pmu_##name##_init(axp_pmu_t *pmu);                                        \
	int pmu_##name##_get_vol(axp_pmu_t *pmu, char *name);                         \
	int pmu_##name##_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff); \
	void pmu_##name##_dump(axp_pmu_t *pmu);

/* AXP PMU defines */
DEFINE_AXP_PMU(axp1530);
DEFINE_AXP_PMU(axp2202);
DEFINE_AXP_PMU(axp2101);
DEFINE_AXP_PMU(axp8191);
DEFINE_AXP_PMU(axp333);

/* Extra */
/**
 * @brief Set the dual phase function on the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp1530_set_dual_phase(axp_pmu_t *pmu);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_AXP_H__
