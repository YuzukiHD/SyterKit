/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file dram-sun65iw1.c
 * @brief DRAM controller driver for the Allwinner sun65iw1 SoC.
 *
 * Provides the DDR/AXP2202 and VDD_SYS voltage hooks plus the microsecond
 * delay helper used by the DRAM init blob, and drives the external DRAM
 * initialization routine.
 */

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

static axp_pmu_t *dram_pmu_axp2202;
static axp_pmu_t *dram_pmu_axp1530;

extern int init_DRAM(int type, void *buff);

/**
 * @brief Set the DDR supply voltage.
 *
 * Programs the AXP1530 dcdc3 rail used for DDR power.
 *
 * @param[in] vol_val Target voltage value in millivolts.
 *
 * @return Result of the AXP1530 voltage programming.
 */
int set_ddr_voltage(uint32_t vol_val)
{
	pr_debug("Setting DDR voltage to %u mV for axp323 dcdc3\n", vol_val);
	return pmu_axp1530_set_vol(dram_pmu_axp1530, "dcdc3", vol_val, 1);
}

/**
 * @brief Identify the PMU that supplies VDD_SYS.
 *
 * Probes the AXP2202 chip ID and falls back to the fallback I2C address if the
 * primary address cannot be read.
 */
void get_vdd_sys_pmu_id(void)
{
	axp_pmu_t *pmu = dram_pmu_axp2202;
	uint8_t axp_val;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202))
		return;
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_CHIP_ID_EXT, &axp_val)) {
		if (pmu->fallback_address == 0U || sunxi_i2c_read(pmu->i2c, pmu->fallback_address, AXP2202_CHIP_ID_EXT, &axp_val)) {
			pr_warn("PMU: AXP2202 PMU Read error\n");
			return;
		}
		pmu->address = pmu->fallback_address;
	}
}

/**
 * @brief Set the VDD_SYS rail voltage and power state.
 *
 * Programs the AXP2202 dcdc2 (DC2OUT) voltage and output control register.
 *
 * @param[in] set_vol Target voltage value in millivolts.
 * @param[in] onoff   Non-zero to enable the rail, zero to disable it.
 *
 * @return 0 on success, -1 on error.
 */
int set_vdd_sys_reg(int set_vol, int onoff)
{
	axp_pmu_t *pmu = dram_pmu_axp2202;
	uint8_t reg_value;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202))
		return -1;

	/* read cfg value */
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_DC2OUT_VOL, &reg_value))
		return -1;

	/* set voltage */
	reg_value &= ~0x7f;
	set_vol &= 0x7f;
	reg_value |= set_vol;
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_DC2OUT_VOL, reg_value))
		return -1;

	/* set on/onff */
	if (sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_OUTPUT_CTL0, &reg_value))
		return -1;

	if (onoff == 0) {
		reg_value &= ~(1 << 1);
	} else {
		reg_value |= (1 << 1);
	}
	if (sunxi_i2c_write(pmu->i2c, pmu->address, AXP2202_OUTPUT_CTL0, reg_value))
		return -1;

	pr_debug("Setting VDD_SYS to %d mV, state: %s\n", pmu_axp2202_get_vol(pmu, "dcdc2"), onoff ? "ON" : "OFF");

	return 0;
}

/**
 * @brief Get the VDD_SYS rail voltage register value.
 *
 * @return Current AXP2202 DC2OUT voltage register value, or -1 on error.
 */
uint8_t get_vdd_sys_reg(void)
{
	axp_pmu_t *pmu = dram_pmu_axp2202;
	uint8_t reg_val = 0;

	if (!axp_pmu_matches(pmu, AXP_PMU_AXP2202) || sunxi_i2c_read(pmu->i2c, pmu->address, AXP2202_DC2OUT_VOL, &reg_val))
		return -1;

	pr_debug("Getting VDD_SYS reg = 0x%x\n", reg_val);
	return reg_val;
}

/**
 * @brief Delay for a number of microseconds.
 *
 * @param[in] us Delay duration in microseconds.
 */
void __usdelay(unsigned long us)
{
	udelay((uint32_t)us);
}

/**
 * @brief Initialize the DRAM controller.
 *
 * Validates the DRAM parameters, saves the AXP2202/AXP1530 PMU handles,
 * identifies the VDD_SYS PMU and delegates to the external init_DRAM routine.
 *
 * @param[in] dram DRAM configuration block.
 *
 * @return Detected DRAM size in bytes, or 0 on failure.
 */
uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram_pmu_axp2202 = dram->pmu;
	dram_pmu_axp1530 = dram->pmu_aux;
	get_vdd_sys_pmu_id();
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
