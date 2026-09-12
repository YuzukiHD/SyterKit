/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun252iw1.h>
#include <uart.h>

#include <common.h>
#include <io.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/serial/serial.h>
#include <efex.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCU_BASE + 0x90c),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCU_BASE + 0x90c),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 9), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 5 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 10), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 5 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static sunxi_dram_t dram = {
	.parameters = {
		1056,
		3,
		0x007b6bfb,
		0x00000001,
		0x000010d2,
		0x00000000,
		0x00001c70,
		0x00000002,
		0x00000018,
		0x00000000,
		0x004a2195,
		0x02423190,
		0x0008b061,
		0xb4787896,
		0x00000000,
		0x48484848,
		0x00000048,
		0x1621121e,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00460000,
		0x00000055,
		0x34040500,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

static sunxi_i2c_t i2c = {
	.base = SUNXI_TWI2_BASE,
	.id = 2,
	.speed = SUNXI_I2C_SPEED_400K,
	.i2c_clk = {
		.gate_reg_base = (SUNXI_CCU_BASE + 0x91c),
		.gate_reg_offset = 2,
		.rst_reg_base = (SUNXI_CCU_BASE + 0x91c),
		.rst_reg_offset = 18,
		.parent_clk = 24000000,
	},
	.gpio = {
		.gpio_scl = { .base = SUNXI_RGPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 3), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
		.gpio_sda = { .base = SUNXI_RGPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 4), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
	},
};

static void sunxi_pmc_config(void)
{
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0)))
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
}

int main(void)
{
	axp_pmu_t pmu;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	if (pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: configuration failed\n");
		return -1;
	}
	sunxi_clk_init();
	sunxi_pmc_config();
	sunxi_i2c_init(&i2c);
	pmu_axp333_init(&pmu);
	pmu_axp333_set_vol(&pmu, "dcdc2", 1500, 1);
	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
