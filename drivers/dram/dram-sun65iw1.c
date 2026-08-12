/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <mmu.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <dt2c/driver.h>
#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>
#include <drivers/pmu/axp.h>

#include <drivers/pmu/reg/reg-axp2202.h>


static axp_pmu_t *dram_primary_pmu;
static axp_pmu_t *dram_secondary_pmu;

extern int init_DRAM(int type, void *buff);

int set_ddr_voltage(uint32_t vol_val) {
	printk_debug("Setting DDR voltage to %u mV for axp323 dcdc3\n", vol_val);
	pmu_axp1530_set_vol(dram_secondary_pmu, "dcdc3", vol_val, 1);
	return 0;
}

void get_vdd_sys_pmu_id(void) {
	axp_pmu_t *pmu = dram_primary_pmu;
	uint8_t axp_val;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202))
		return;
	if (sunxi_i2c_read(pmu->i2c, pmu->address,
			   AXP2202_CHIP_ID_EXT, &axp_val)) {
		if (pmu->fallback_address == 0U ||
		    sunxi_i2c_read(pmu->i2c, pmu->fallback_address,
				   AXP2202_CHIP_ID_EXT, &axp_val)) {
			printk_warning("PMU: AXP2202 PMU Read error\n");
			return;
		}
		pmu->address = pmu->fallback_address;
	}
}

int set_vdd_sys_reg(int set_vol, int onoff) {
	axp_pmu_t *pmu = dram_primary_pmu;
	uint8_t reg_value;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202))
		return -1;

	/* read cfg value */
	if (sunxi_i2c_read(pmu->i2c, pmu->address,
			   AXP2202_DC2OUT_VOL, &reg_value))
		return -1;

	/* set voltage */
	reg_value &= ~0x7f;
	set_vol &= 0x7f;
	reg_value |= set_vol;
	sunxi_i2c_write(pmu->i2c, pmu->address,
			AXP2202_DC2OUT_VOL, reg_value);

	/* set on/onff */
	if (sunxi_i2c_read(pmu->i2c, pmu->address,
			   AXP2202_OUTPUT_CTL0, &reg_value))
		return -1;

	if (onoff == 0) {
		reg_value &= ~(1 << 1);
	} else {
		reg_value |= (1 << 1);
	}
	sunxi_i2c_write(pmu->i2c, pmu->address,
			AXP2202_OUTPUT_CTL0, reg_value);

	printk_debug("Setting VDD_SYS to %d mV, state: %s\n",
		     pmu_axp2202_get_vol(pmu, "dcdc2"),
		     onoff ? "ON" : "OFF");

	return 0;
}

uint8_t get_vdd_sys_reg(void) {
	axp_pmu_t *pmu = dram_primary_pmu;
	uint8_t reg_val = 0;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202) ||
	    sunxi_i2c_read(pmu->i2c, pmu->address,
			   AXP2202_DC2OUT_VOL, &reg_val))
		return -1;

	printk_debug("Getting VDD_SYS reg = 0x%x\n", reg_val);
	return reg_val;
}

void __usdelay(unsigned long us) {
	udelay((uint32_t) us);
}


uint32_t sunxi_dram_init(sunxi_dram_t *dram) {
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram_primary_pmu = dram->primary_pmu;
	dram_secondary_pmu = dram->secondary_pmu;
	get_vdd_sys_pmu_id();
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sun65iw1-dram");
