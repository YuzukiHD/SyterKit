/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <pmu/axp.h>

/**
 * Get control information from the table based on the given name.
 *
 * @param name The name of the control information to retrieve.
 * @return A pointer to the axp_contrl_info structure corresponding to the given name,
 *         or NULL if the name is not found in the table.
 */
static axp_contrl_info *get_ctrl_info_from_tbl(char *name, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size) {
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

int axp_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size, uint8_t axp_addr) {
	uint8_t reg_value, i;
	axp_contrl_info *p_item = NULL;
	uint8_t base_step = 0;

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

		if (sunxi_i2c_read(i2c_dev, axp_addr, p_item->cfg_reg_addr, &reg_value)) {
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
				base_step += ((p_item->axp_step_tbl[i].step_max_vol - p_item->axp_step_tbl[i].step_min_vol + p_item->axp_step_tbl[i].step_val) / p_item->axp_step_tbl[i].step_val);
			}
		}

		if (sunxi_i2c_write(i2c_dev, axp_addr, p_item->cfg_reg_addr, reg_value)) {
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (sunxi_i2c_read(i2c_dev, axp_addr, p_item->ctrl_reg_addr, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (sunxi_i2c_write(i2c_dev, axp_addr, p_item->ctrl_reg_addr, reg_value)) {
		return -1;
	}
	return 0;
}

int axp_get_vol(sunxi_i2c_t *i2c_dev, char *name, axp_contrl_info *axp_ctrl_tbl, uint8_t axp_ctrl_tbl_size, uint8_t axp_addr) {
	uint8_t reg_value, i;
	axp_contrl_info *p_item = NULL;
	uint8_t base_step1 = 0;
	uint8_t base_step2 = 0;
	int vol;

	p_item = get_ctrl_info_from_tbl(name, axp_ctrl_tbl, axp_ctrl_tbl_size);
	if (!p_item) {
		return -1;
	}

	if (sunxi_i2c_read(i2c_dev, axp_addr, p_item->ctrl_reg_addr, &reg_value)) {
		return -1;
	}

	if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs))) {
		return 0;
	}

	if (sunxi_i2c_read(i2c_dev, axp_addr, p_item->cfg_reg_addr, &reg_value)) {
		return -1;
	}

	printk_trace("PMU: %s reg_val = 0x%x\n", name, reg_value);
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