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

int pmu_axp1530_init(sunxi_i2c_t *i2c_dev);

int pmu_axp1530_get_vol(sunxi_i2c_t *i2c_dev, char *name);

int pmu_axp1530_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff);

void pmu_axp1530_dump(sunxi_i2c_t *i2c_dev);

#endif // __G_AXP_H__