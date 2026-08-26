/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>

#include <common.h>
#include <stdlib.h>

#include <dt-bindings/soc/sun65iw1.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/sid-dt.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>
#include <string.h>

static sunxi_dram_t dram;

extern sunxi_serial_t uart_dbg;

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv)
{
	dump_stack();
	return 0;
}

msh_declare_command(ddr_test);
msh_define_help(ddr_test, "ddr w/r test", "Usage: ddr_test\n");
int cmd_ddr_test(int argc, const char **argv)
{
	dump_hex(dram.memory_base, 0x100);
	memset((void *)dram.memory_base, 0x5A, 0x2000);
	dump_hex(dram.memory_base, 0x100);
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(bt),
	msh_define_command(ddr_test),
	msh_command_end,
};

/* Resource Control Register Configuration Macros */
#define RES_CTRL_BASE_VAL 0x19190000 /* Base value for resource control registers */
#define RES_VAL_MASK 0xF /* Resource configuration value mask */

/**
 * @brief Initialize resource controller
 * 
 * Read resource configuration values from SID registers and set corresponding
 * resource control registers including DSI, CSI, USB, EDP, HS_COMBO, and DDR
 */
static void sunxi_res_ctrl_init(const sunxi_sid_t *sid)
{
	/* Read resource configuration field from SID register */
	uint8_t sid_res_value = (uint8_t)(sunxi_efuse_sram_read(sid, 0x40U) >> 24);

	/* No initialization needed if resource configuration value is 0 */
	if (sid_res_value == 0) {
		return;
	}

	/* Extract res0 configuration value (lower 4 bits) and configure corresponding resources */
	uint8_t res0_value = sid_res_value & RES_VAL_MASK;
	writel(RES_CTRL_BASE_VAL | res0_value, INT_DSI_RES_CTRL_REG);
	writel(RES_CTRL_BASE_VAL | res0_value, INT_CSI_RES_CTRL_REG);
	writel(RES_CTRL_BASE_VAL | res0_value, INT_USB_RES_CTRL_REG);

	/* Extract res1 configuration value (upper 4 bits) and configure corresponding resources */
	uint8_t res1_value = (sid_res_value >> 4) & RES_VAL_MASK;
	writel(RES_CTRL_BASE_VAL | res1_value, INT_EDP_RES_CTRL_REG);
	writel(RES_CTRL_BASE_VAL | res1_value, INT_HS_COMBO_RES_CTRL_REG);
	writel(RES_CTRL_BASE_VAL | res1_value, INT_DDR_RES_CTRL_REG);
}

/* Voltage Configuration Related Macros */
#define DEFAULT_SYS_VOLTAGE 900 /* Default system voltage 0.9V */
#define DEFAULT_GPU_VOLTAGE 940 /* Default GPU voltage 0.94V */
#define VDD_DCDC1_VOLTAGE 1050 /* DCDC1 voltage 1.05V */
#define VDD_3V3_VOLTAGE 3300 /* 3.3V voltage */
#define VDD_1V8_VOLTAGE 1800 /* 1.8V voltage */

/* EFUSE Related Register Offset and Mask */
#define EFUSE_SRAM_OFFSET 0x14 /* EFUSE SRAM mirror offset */
#define EFUSE_MASK 0xFF0000 /* EFUSE mask */
#define EFUSE_EXT_MASK 0xFF000000 /* Extended EFUSE mask */
#define EFUSE_SHIFT 16 /* EFUSE shift */
#define EFUSE_EXT_SHIFT 24 /* Extended EFUSE shift */

/**
 * @brief Voltage configuration structure
 */
typedef struct {
	uint32_t sys_voltage; /* System voltage value (mV) */
	uint32_t gpu_voltage; /* GPU voltage value (mV) */
} voltage_config_t;

/**
 * @brief Get voltage configuration based on EFUSE value
 * 
 * @param efuse_value Value read from EFUSE register
 * @return voltage_config_t Voltage configuration structure
 */
static voltage_config_t get_voltage_config(uint8_t efuse_value)
{
	voltage_config_t config = { .sys_voltage = DEFAULT_SYS_VOLTAGE, .gpu_voltage = DEFAULT_GPU_VOLTAGE };

	/* Determine voltage configuration based on EFUSE value */
	switch (efuse_value) {
	case 0x00:
	case 0x24:
	case 0x03:
		/* Use default values, no modification needed */
		break;
	case 0x01:
		config.gpu_voltage = 980;
		break;
	case 0x44:
		config.gpu_voltage = 900;
		break;
	case 0x34:
		config.sys_voltage = 920;
		config.gpu_voltage = 960;
		break;
	/* Use default voltage configuration for other cases */
	default:
		break;
	}

	return config;
}

/**
 * @brief Initialize board power system
 * 
 * 1. Read EFUSE value to determine voltage configuration
 * 2. Initialize I2C and PMU chips
 * 3. Configure each power rail voltage based on SOC version and EFUSE value
 */
static void sunxi_board_power_init(const sunxi_sid_t *sid, sunxi_i2c_t *i2c, axp_pmu_t *axp2202, axp_pmu_t *axp1530)
{
	/* Read EFUSE value to determine voltage configuration */
	uint32_t efuse_reg_value = sunxi_efuse_sram_read(sid, EFUSE_SRAM_OFFSET);
	uint8_t efuse_value = (uint8_t)((efuse_reg_value & EFUSE_MASK) >> EFUSE_SHIFT);
	uint8_t efuse_ext_value = (uint8_t)((efuse_reg_value & EFUSE_EXT_MASK) >> EFUSE_EXT_SHIFT);

	/* Use extended EFUSE value if available */
	if (efuse_ext_value) {
		efuse_value = efuse_ext_value;
	}

	/* Get voltage configuration */
	voltage_config_t volt_config = get_voltage_config(efuse_value);

	/* Initialize I2C controller and PMU chips */
	sunxi_i2c_init(i2c);
	pmu_axp2202_init(axp2202);
	pmu_axp1530_init(axp1530);

	/* For early SOC versions, system voltage should equal GPU voltage */
	if ((readl(SUNXI_SOC_VER_REG) & SUNXI_SOC_VER_MASK) < 2) {
		volt_config.sys_voltage = volt_config.gpu_voltage;
	}

	/* Configure PMU AXP2202 voltages */
	pmu_axp2202_set_vol(axp2202, "dcdc1", VDD_DCDC1_VOLTAGE, 1);
	pmu_axp2202_set_vol(axp2202, "dcdc2", volt_config.sys_voltage, 1);
	pmu_axp2202_set_vol(axp2202, "dcdc4", VDD_3V3_VOLTAGE, 1);
	pmu_axp2202_set_vol(axp2202, "bldo3", VDD_1V8_VOLTAGE, 1);

	/* Configure PMU AXP1530 voltages */
	pmu_axp1530_set_vol(axp1530, "dcdc1", 1000, 1); /* Fixed 1.0V */
	pmu_axp1530_set_vol(axp1530, "dcdc2", 1000, 1); /* Fixed 1.0V */
	pmu_axp1530_set_vol(axp1530, "dcdc3", volt_config.gpu_voltage, 1);
}

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_sid_t sid;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_res_ctrl_init(&sid);

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK || pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_board_power_init(&sid, &i2c, &axp2202, &axp1530);

	dram.pmu = &axp2202;
	dram.pmu_aux = &axp1530;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
