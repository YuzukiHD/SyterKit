/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun65iw1.h>
#include <uart.h>

#include <common.h>
#include <io.h>
#include <log.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/serial/serial.h>
#include <drivers/sid/sid.h>
#include <efex.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCM_BASE + 0x600),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCM_BASE + 0x600),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 10), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 11), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static sunxi_dram_t dram = {
	.parameters = {
		0x000003f0,
		8,
		0x07070707,
		0x0c0c0c0c,
		0x000c0c0c,
		0x59595b5d,
		0x0000310a,
		0x00001000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000033,
		0x00000003,
		0x00000000,
		0x00000000,
		0x00000066,
		0x00000030,
		0x00000000,
		0x00000020,
		0x00000000,
		0x00000000,
		0x00000002,
		0x00000000,
		0x28282828,
		0x00000000,
		0x00000000,
		0x103f353f,
		0x00004b00,
		0x65656667,
		0x18161718,
		0x00000051,
		0x010701f5,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

static sunxi_i2c_t i2c = {
	.base = SUNXI_S_TWI0_BASE,
	.id = 6,
	.speed = SUNXI_I2C_SPEED_400K,
	.i2c_clk = {
		.gate_reg_base = (SUNXI_RPRCM_BASE + 0x19c),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_RPRCM_BASE + 0x19c),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio = {
		.gpio_scl = { .base = SUNXI_S_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 0), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
		.gpio_sda = { .base = SUNXI_S_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 1), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
	},
};

static const sunxi_sid_t sid = {
	.base = SUNXI_SID_BASE,
	.size = 0x400,
	.sram_base = SUNXI_SID_BASE + SUNXI_SID_SRAM_OFFSET,
};

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

static void sunxi_power_init(const sunxi_sid_t *sid, sunxi_i2c_t *i2c, axp_pmu_t *axp2202, axp_pmu_t *axp1530)
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

	sunxi_res_ctrl_init(&sid);
	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	if (pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK || pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK) {
		pr_err("PMU: configuration failed\n");
		return -1;
	}
	sunxi_power_init(&sid, &i2c, &axp2202, &axp1530);
	dram.power.vdd_sys = &axp2202;
	dram.power.ddr = &axp1530;
	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
