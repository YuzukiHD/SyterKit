/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun55iw3.h>
#include <uart.h>

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/clk/sun55iw3/clk.h>
#include <drivers/clk/sun55iw3/reg.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>

#include <efex.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCM_BASE + 0x90c),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCM_BASE + 0x90c),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 9), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 10), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static sunxi_dram_t dram = {
	.parameters = {
		0x000004b0,
		8,
		0x07070707,
		0x0d0d0d0d,
		0x00000e0e,
		0x84848484,
		0x0000310a,
		0x08000000,
		0x00000000,
		0x00000034,
		0x0000001b,
		0x00000033,
		0x00000003,
		0x00000000,
		0x00000000,
		0x00000004,
		0x00000072,
		0x00000000,
		0x00000008,
		0x00000000,
		0x00000000,
		0x00000026,
		0x80808080,
		0x06060606,
		0x00000000,
		0x74000000,
		0x38000000,
		0x802f3333,
		0xc7c5c4c2,
		0x3533302f,
		0x00000860,
		0x48484848,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

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
		.gpio_scl = { .base = SUNXI_R_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 0), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
		.gpio_sda = { .base = SUNXI_R_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 1), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 2 },
	},
};

static sunxi_remoteproc_t e906 = {
	.format = SUNXI_REMOTEPROC_FIRMWARE_ELF32,
	.firmware = {
		{ .name = "e906.bin", .load_address = 0x48100000, .region_size = 0x01000000 },
	},
	.firmware_count = 1,
	.registers = {
		{ .base = SUNXI_DSP_PRCM_BASE, .size = 0x1000 },
		{ .base = SUNXI_RISCV_CFG_BASE, .size = 0x1000 },
	},
	.register_count = 2,
	.ops = &sunxi_remoteproc_ops,
};

static void soc_set_rpio_power_mode(void)
{
	if (read32(SUNXI_R_GPIO_BASE + 0x348) & 1)
		write32(SUNXI_R_GPIO_BASE + 0x340, 1);
}

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();

	show_banner();
	if (pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK || pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK) {
		pr_err("PMU: configuration failed\n");
		return -1;
	}

	sunxi_clk_init();

	soc_set_rpio_power_mode();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&axp2202);

	pmu_axp1530_init(&axp1530);

	pmu_axp2202_set_vol(&axp2202, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&axp1530);
	pmu_axp1530_set_vol(&axp1530, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&axp1530, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&axp2202, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc4", 3300, 1);

	pmu_axp2202_set_vol(&axp2202, "bldo3", 1800, 1);
	pmu_axp2202_set_vol(&axp2202, "bldo1", 1800, 1);

	pmu_axp2202_dump(&axp2202);
	pmu_axp1530_dump(&axp1530);

	sun55iw3_clk_set_cpu_pll(1800);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		pr_err("RISC-V E906: reset failed\n");
		return -1;
	}

	uint32_t dram_size = sunxi_dram_init(&dram);
	if (dram_size == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	pr_info("DRAM: DRAM Size = %dMB", dram_size);

	/* PLL DDR0 */
	uint32_t reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_DDR0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		uint32_t plln = ((reg32 >> 8) & 0xff) + 1;

		uint32_t p1 = ((reg32 >> 1) & 0x1) + 1;
		uint32_t p0 = (reg32 & 0x01) + 1;

		printk(LOG_LEVEL_MUTE, ", DRAM CLK = %luMHz", (24 * plln) / (p0 * p1));
	}

	printk(LOG_LEVEL_MUTE, "\n");

	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
