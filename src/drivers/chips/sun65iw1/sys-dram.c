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

#include <reg-ncat.h>

#include <sys-dram.h>
#include <sys-rtc.h>
#include <pmu/axp.h>

#include <pmu/reg/reg-axp2202.h>

extern sunxi_i2c_t i2c_pmu;

static uint32_t AXP2202_RUNTIME_ADDR = 0x0;
static uint32_t dram_size;

extern int init_DRAM(int type, void *buff);

int set_ddr_voltage(uint32_t vol_val) {
	printk_debug("Setting DDR voltage to %u mV for axp323 dcdc3\n", vol_val);
	pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", vol_val, 1);
	return 0;
}

void get_vdd_sys_pmu_id(void) {
	uint8_t axp_val;
	/* Try to probe AXP717B */
	if (sunxi_i2c_read(&i2c_pmu, AXP2202_B_RUNTIME_ADDR, AXP2202_CHIP_ID_EXT, &axp_val)) {
		/* AXP717B probe fail, Try to probe AXP717C */
		if (sunxi_i2c_read(&i2c_pmu, AXP2202_C_RUNTIME_ADDR, AXP2202_CHIP_ID_EXT, &axp_val)) {
			/* AXP717C probe fail */
			printk_warning("PMU: AXP2202 PMU Read error\n");
			return;
		} else {
			AXP2202_RUNTIME_ADDR = AXP2202_C_RUNTIME_ADDR;
		}
	} else {
		AXP2202_RUNTIME_ADDR = AXP2202_B_RUNTIME_ADDR;
	}
}

int set_vdd_sys_reg(int set_vol, int onoff) {
	uint8_t reg_value;

	/* read cfg value */
	if (sunxi_i2c_read(&i2c_pmu, AXP2202_RUNTIME_ADDR, AXP2202_DC2OUT_VOL, &reg_value))
		return -1;

	/* set voltage */
	reg_value &= ~0x7f;
	set_vol &= 0x7f;
	reg_value |= set_vol;
	sunxi_i2c_write(&i2c_pmu, AXP2202_RUNTIME_ADDR, AXP2202_DC2OUT_VOL, reg_value);

	/* set on/onff */
	if (sunxi_i2c_read(&i2c_pmu, AXP2202_RUNTIME_ADDR, AXP2202_OUTPUT_CTL0, &reg_value))
		return -1;

	if (onoff == 0) {
		reg_value &= ~(1 << 1);
	} else {
		reg_value |= (1 << 1);
	}
	sunxi_i2c_write(&i2c_pmu, AXP2202_RUNTIME_ADDR, AXP2202_OUTPUT_CTL0, reg_value);

	printk_debug("Setting VDD_SYS to %d mV, state: %s\n", pmu_axp2202_get_vol(&i2c_pmu, "dcdc2"), onoff ? "ON" : "OFF");

	return 0;
}

uint8_t get_vdd_sys_reg(void) {
	uint8_t reg_val = 0;

	if (sunxi_i2c_read(&i2c_pmu, AXP2202_RUNTIME_ADDR, AXP2202_DC2OUT_VOL, &reg_val))
		return -1;

	printk_debug("Getting VDD_SYS reg = 0x%x\n", reg_val);
	return reg_val;
}

void __usdelay(unsigned long us) {
	udelay((uint32_t) us);
}


uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	get_vdd_sys_pmu_id();
	dram_size = init_DRAM(0, para);
	return dram_size;
}
