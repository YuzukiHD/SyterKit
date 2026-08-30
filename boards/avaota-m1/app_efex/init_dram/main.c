/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <io.h>
#include <log.h>
#include <dt-bindings/soc/sun65iw1.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/serial/serial.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/sid-dt.h>
#include <efex.h>

static sunxi_dram_t dram;

static void sunxi_res_ctrl_init(const sunxi_sid_t *sid)
{
	uint8_t value = (uint8_t)(sunxi_efuse_sram_read(sid, 0x40U) >> 24);
	uint32_t res0;
	uint32_t res1;

	if (value == 0U)
		return;
	res0 = 0x19190000U | (value & 0xfU);
	res1 = 0x19190000U | ((value >> 4) & 0xfU);
	writel(res0, INT_DSI_RES_CTRL_REG);
	writel(res0, INT_CSI_RES_CTRL_REG);
	writel(res0, INT_USB_RES_CTRL_REG);
	writel(res1, INT_EDP_RES_CTRL_REG);
	writel(res1, INT_HS_COMBO_RES_CTRL_REG);
	writel(res1, INT_DDR_RES_CTRL_REG);
}

static void sunxi_board_power_init(const sunxi_sid_t *sid, sunxi_i2c_t *i2c,
				   axp_pmu_t *axp2202, axp_pmu_t *axp1530)
{
	uint32_t efuse = sunxi_efuse_sram_read(sid, 0x14U);
	uint8_t value = (uint8_t)(efuse >> 16);
	uint8_t extended = (uint8_t)(efuse >> 24);
	uint32_t sys_mv = 900;
	uint32_t gpu_mv = 940;

	if (extended != 0U)
		value = extended;
	if (value == 0x01U)
		gpu_mv = 980;
	else if (value == 0x44U)
		gpu_mv = 900;
	else if (value == 0x34U) {
		sys_mv = 920;
		gpu_mv = 960;
	}
	sunxi_i2c_init(i2c);
	pmu_axp2202_init(axp2202);
	pmu_axp1530_init(axp1530);
	if ((readl(SUNXI_SOC_VER_REG) & SUNXI_SOC_VER_MASK) < 2U)
		sys_mv = gpu_mv;
	pmu_axp2202_set_vol(axp2202, "dcdc1", 1050, 1);
	pmu_axp2202_set_vol(axp2202, "dcdc2", sys_mv, 1);
	pmu_axp2202_set_vol(axp2202, "dcdc4", 3300, 1);
	pmu_axp2202_set_vol(axp2202, "bldo3", 1800, 1);
	pmu_axp1530_set_vol(axp1530, "dcdc1", 1000, 1);
	pmu_axp1530_set_vol(axp1530, "dcdc2", 1000, 1);
	pmu_axp1530_set_vol(axp1530, "dcdc3", gpu_mv, 1);
}

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_sid_t sid;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK)
		return -1;
	sunxi_res_ctrl_init(&sid);
	if (sunxi_serial_init_stdout() != 0)
		return -1;
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK ||
	    pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_board_power_init(&sid, &i2c, &axp2202, &axp1530);
	dram.power.vdd_sys = &axp2202;
	dram.power.ddr = &axp1530;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK ||
	    sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
