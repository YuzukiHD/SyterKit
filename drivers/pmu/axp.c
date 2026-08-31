/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "axp: " fmt

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <driver.h>
#include <log.h>

#include <drivers/pmu/axp.h>
#include <string.h>

/**
 * @brief Get control information from the table based on the given name.
 *
 * @param name The name of the control information to retrieve.
 * @return A pointer to the axp_contrl_info structure corresponding to the given name,
 *         or NULL if the name is not found in the table.
 */
static axp_contrl_info *get_ctrl_info_from_tbl(char *name, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size)
{
	int i = 0;
	for (i = 0; i < axp_ctrl_tbl_size; i++) {
		if (!strncmp(name, axp_ctrl_tbl[i].name, strlen(axp_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= axp_ctrl_tbl_size) {
		return NULL;
	}
	return (axp_ctrl_tbl + i);
}

/**
 * @brief Set the voltage and on/off state of an AXP rail
 * @details Looks up the control item for the named rail, clamps the requested
 *          voltage to the valid range, and computes the configuration register
 *          value from the rail's step table. If set_vol is positive the voltage
 *          register is written; if onoff is not negative the rail's enable bit
 *          is also programmed in the control register.
 * @param pmu AXP PMIC device handle
 * @param name Name of the rail to configure
 * @param set_vol Requested voltage in millivolts, or 0 to skip voltage programming
 * @param onoff Enable (1), disable (0), or leave power state unchanged (-1)
 * @param axp_ctrl_tbl Table of AXP control items describing the rail registers
 * @param axp_ctrl_tbl_size Number of entries in axp_ctrl_tbl
 * @return 0 on success, -1 if the rail is unknown or an I2C access fails
 */
int axp_set_vol(axp_pmu_t *pmu, char *name, int set_vol, int onoff, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size)
{
	uint8_t reg_value, i;
	axp_contrl_info *p_item = NULL;
	uint8_t base_step = 0;

	if (pmu == NULL || pmu->i2c == NULL || pmu->address == 0U)
		return -1;

	p_item = get_ctrl_info_from_tbl(name, axp_ctrl_tbl, axp_ctrl_tbl_size);
	if (!p_item) {
		return -1;
	}

	if ((set_vol > 0) && (p_item->min_vol)) {
		if (set_vol < p_item->min_vol) {
			set_vol = p_item->min_vol;
		} else if (set_vol > p_item->max_vol) {
			set_vol = p_item->max_vol;
		}

		if (sunxi_i2c_read(pmu->i2c, pmu->address, p_item->cfg_reg_addr, &reg_value)) {
			return -1;
		}

		reg_value &= ~p_item->cfg_reg_mask;

		for (i = 0; p_item->axp_step_tbl[i].step_max_vol != 0; i++) {
			if ((set_vol > p_item->axp_step_tbl[i].step_max_vol) && (set_vol < p_item->axp_step_tbl[i + 1].step_min_vol)) {
				set_vol = p_item->axp_step_tbl[i].step_max_vol;
			}
			if (p_item->axp_step_tbl[i].step_max_vol >= set_vol) {
				reg_value |= ((base_step + ((set_vol - p_item->axp_step_tbl[i].step_min_vol) / p_item->axp_step_tbl[i].step_val)) << p_item->reg_addr_offset);
				if (p_item->axp_step_tbl[i].regation) {
					uint8_t reg_value_temp = (~reg_value & p_item->cfg_reg_mask);
					reg_value &= ~p_item->cfg_reg_mask;
					reg_value |= reg_value_temp;
				}
				break;
			} else {
				base_step += ((p_item->axp_step_tbl[i].step_max_vol - p_item->axp_step_tbl[i].step_min_vol + p_item->axp_step_tbl[i].step_val) /
					      p_item->axp_step_tbl[i].step_val);
			}
		}

		if (sunxi_i2c_write(pmu->i2c, pmu->address, p_item->cfg_reg_addr, reg_value)) {
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (sunxi_i2c_read(pmu->i2c, pmu->address, p_item->ctrl_reg_addr, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (sunxi_i2c_write(pmu->i2c, pmu->address, p_item->ctrl_reg_addr, reg_value)) {
		return -1;
	}
	return 0;
}

/**
 * @brief Read the current voltage of an AXP rail
 * @details Looks up the control item for the named rail, checks the rail's enable
 *          bit, reads the configuration register, and converts the raw register
 *          value back to millivolts using the rail's step table.
 * @param pmu AXP PMIC device handle
 * @param name Name of the rail to query
 * @param axp_ctrl_tbl Table of AXP control items describing the rail registers
 * @param axp_ctrl_tbl_size Number of entries in axp_ctrl_tbl
 * @return Current rail voltage in millivolts, 0 if the rail is disabled, or -1 on error
 */
int axp_get_vol(axp_pmu_t *pmu, char *name, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size)
{
	uint8_t reg_value, i;
	axp_contrl_info *p_item = NULL;
	uint8_t base_step1 = 0;
	uint8_t base_step2 = 0;
	int vol;

	if (pmu == NULL || pmu->i2c == NULL || pmu->address == 0U)
		return -1;

	p_item = get_ctrl_info_from_tbl(name, axp_ctrl_tbl, axp_ctrl_tbl_size);
	if (!p_item) {
		return -1;
	}

	if (sunxi_i2c_read(pmu->i2c, pmu->address, p_item->ctrl_reg_addr, &reg_value)) {
		return -1;
	}

	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}

	if (sunxi_i2c_read(pmu->i2c, pmu->address, p_item->cfg_reg_addr, &reg_value)) {
		return -1;
	}

	pr_trace("%s reg_val = 0x%x\n", name, reg_value);
	reg_value &= p_item->cfg_reg_mask;
	reg_value >>= p_item->reg_addr_offset;
	for (i = 0; p_item->axp_step_tbl[i].step_max_vol != 0; i++) {
		base_step1 += ((p_item->axp_step_tbl[i].step_max_vol - p_item->axp_step_tbl[i].step_min_vol + p_item->axp_step_tbl[i].step_val) / p_item->axp_step_tbl[i].step_val);
		if (reg_value < base_step1) {
			vol = (reg_value - base_step2) * p_item->axp_step_tbl[i].step_val + p_item->axp_step_tbl[i].step_min_vol;
			return vol;
		}
		base_step2 += ((p_item->axp_step_tbl[i].step_max_vol - p_item->axp_step_tbl[i].step_min_vol + p_item->axp_step_tbl[i].step_val) / p_item->axp_step_tbl[i].step_val);
	}
	return -1;
}
