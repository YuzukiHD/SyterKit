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

typedef struct _axp_step_info {
    uint32_t step_min_vol;
    uint32_t step_max_vol;
    uint32_t step_val;
    uint32_t regation;
} axp_step_info_t;

typedef struct _axp_contrl_info {
    char name[16];
    uint32_t min_vol;
    uint32_t max_vol;
    uint32_t cfg_reg_addr;
    uint32_t cfg_reg_mask;
    uint32_t ctrl_reg_addr;
    uint32_t ctrl_bit_ofs;
    uint32_t reg_addr_offest;
    axp_step_info_t axp_step_tbl[4];
} axp_contrl_info;

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
 * Dump the register values of the AXP1530 PMU.
 *
 * @param i2c_dev Pointer to the I2C device structure.
 */
void pmu_axp1530_dump(sunxi_i2c_t *i2c_dev);



#endif // __G_AXP_H__