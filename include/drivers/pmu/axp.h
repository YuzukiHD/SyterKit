#ifndef __G_AXP_H__
#define __G_AXP_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <sys-i2c.h>

#include "reg-axp.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Structure describing a voltage step of the power domain.
 */
typedef struct _axp_step_info {
    uint32_t step_min_vol;// Minimum voltage level for the step.
    uint32_t step_max_vol;// Maximum voltage level for the step.
    uint32_t step_val;    // Voltage increment value for the step.
    uint32_t regation;    // Regulator register address.
} axp_step_info_t;

/**
 * Structure describing the control information of a power domain.
 */
typedef struct _axp_contrl_info {
    char name[8];                   // Name of the power domain.
    uint32_t min_vol;               // Minimum voltage level for the domain.
    uint32_t max_vol;               // Maximum voltage level for the domain.
    uint32_t cfg_reg_addr;          // Configuration register address.
    uint32_t cfg_reg_mask;          // Configuration register mask.
    uint32_t ctrl_reg_addr;         // Control register address.
    uint32_t ctrl_bit_ofs;          // Bit offset in the control register.
    uint32_t reg_addr_offset;       // Offset of the register address.
    axp_step_info_t axp_step_tbl[4];// Voltage step table for the domain.
} axp_contrl_info;

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
int axp_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size, uint8_t axp_addr);

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
int axp_get_vol(sunxi_i2c_t *i2c_dev, char *name, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size, uint8_t axp_addr);

/* AXP1530 */

/**
 * Initialize the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp1530_init(sunxi_i2c_t *i2c_dev);

/**
 * Get the voltage value of a specific power domain from the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @return The voltage value of the power domain, or -1 if an error occurred.
 */
int pmu_axp1530_get_vol(sunxi_i2c_t *i2c_dev, char *name);

/**
 * Set the voltage value of a specific power domain on the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @param set_vol Voltage value to set.
 * @param onoff On/Off switch for the power domain (1 - On, 0 - Off).
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp1530_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff);

/**
 * Set the dual phase function on the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp1530_set_dual_phase(sunxi_i2c_t *i2c_dev);

/**
 * Dump the register values of the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 */
void pmu_axp1530_dump(sunxi_i2c_t *i2c_dev);

/* AXP2202 */

/**
 * Initialize the AXP2202 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp2202_init(sunxi_i2c_t *i2c_dev);

/**
 * Get the voltage value of a specific power domain from the AXP2202 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @return The voltage value of the power domain, or -1 if an error occurred.
 */
int pmu_axp2202_get_vol(sunxi_i2c_t *i2c_dev, char *name);

/**
 * Set the voltage value of a specific power domain on the AXP2202 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 * @param name Name of the power domain.
 * @param set_vol Voltage value to set.
 * @param onoff On/Off switch for the power domain (1 - On, 0 - Off).
 * @return 0 if successful, -1 if an error occurred.
 */
int pmu_axp2202_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff);

/**
 * Dump the register values of the AXP2202 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 */
void pmu_axp2202_dump(sunxi_i2c_t *i2c_dev);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_AXP_H__