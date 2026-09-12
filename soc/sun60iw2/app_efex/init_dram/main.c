/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun60iw2.h>
#include <uart.h>

#include <common.h>
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
		.gate_reg_base = (SUNXI_CCU_BASE + 0xe00),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCU_BASE + 0xe00),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 9), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
		.gpio_rx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 10), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

#ifdef CONFIG_EFEX_DRAM_LPDDR4
static sunxi_dram_t dram = {
	.parameters = {
		1800,
		8,
		0x08080808,
		0x0e0e0e0e,
		0x88030e0e,
		0,
		0x311a,
		0x1001,
		0x0,
		0x8c,
		0,
		0x33,
		0x0,
		0,
		0,
		0x4,
		0x72,
		0x8,
		0x1d,
		0,
		0,
		0x24,
		0,
		0,
		0x11080503,
		0x200000,
		0x402a,
		0x721f0000,
		0,
		0,
		0x64,
		0,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

#else
static sunxi_dram_t dram = {
	.parameters = {
		0x00000960,
		9,
		0x0e0e0e0e,
		0x0f0f0f0f,
		0xec030e0f,
		0x00000000,
		0x0000a10a,
		0x00001001,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000006,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000012,
		0x00000044,
		0x00000000,
		0x00000034,
		0x00000000,
		0x00000006,
		0x00000000,
		0x00004040,
		0x00000000,
		0x0170b070,
		0x00003800,
		0x00003514,
		0x325f0000,
		0x00000000,
		0x00000000,
		0x00010061,
		0x00000000,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

#endif

static sunxi_i2c_t i2c = {
	.base = SUNXI_R_TWI0_BASE,
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
		.gpio_scl = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 0), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
		.gpio_sda = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 1), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
	},
};

static void soc_common_init(void)
{
	uint32_t version = readl(SUNXI_SOC_VER_REG) & SUNXI_SOC_VER_MASK;

	if (version == 1U)
		writel(0x01155550, SUNXI_PIO_BASE + GPIO_POW_MODE_REG);
	if (version < 2U) {
		uint32_t value = version == 0U ? 0xa7070025 : 0xa7060025;

		writel(value, PLL_LDO_REG);
		writel(value, PLL_LDO_REG);
	}
}

int main(void)
{
	axp_pmu_t pmu;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	if (pmu_axp8191_config(&pmu, &i2c) != DRIVER_OK)
		return -1;
	soc_common_init();
	sunxi_i2c_init(&i2c);
	sunxi_clk_init();
	pmu_axp8191_init(&pmu);
	dram.power.ddr = &pmu;
	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
