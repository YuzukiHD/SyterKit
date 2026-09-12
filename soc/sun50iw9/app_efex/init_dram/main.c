/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun50iw9.h>
#include <uart.h>

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <efex.h>
#include <drivers/sid/sid.h>

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
		.gpio_tx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 0), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 2 },
		.gpio_rx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 1), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 2 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static sunxi_dram_t dram = {
	.parameters = {
		0x000002d0,
		8,
		0x0c0c0c0c,
		0x0e0e0e0e,
		0x00000a0e,
		0x7887ffff,
		0x000030fa,
		0x04000000,
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
		0x00000009,
		0x00000000,
		0x00000000,
		0x00000024,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x39808080,
		0x402f6603,
		0x20262620,
		0x0e0e0f0f,
		0x00001024,
		0x00000000,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
	.init_code_base = 0x00048000,
	.init_code_size = 0x00008000,
	.rtc = { .data_base = SUNXI_RTC_DATA_BASE, .data_size = 0x100 },
};

static sunxi_i2c_t i2c = {
	.base = SUNXI_RTWI_BASE,
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
		.gpio_scl = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 0), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 3 },
		.gpio_sda = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 1), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 3 },
	},
};

static const sunxi_sid_t sid = {
	.base = SUNXI_SID_BASE,
	.size = 0x400,
	.sram_base = SUNXI_SID_BASE + SUNXI_SID_SRAM_OFFSET,
};

static void set_cpu_down(unsigned int cpu)
{
	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_DBG_REG1, 1 << cpu);
	udelay(10);
	setbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CLUSTER_PWROFF_GATING, 1 << cpu);
	udelay(20);
	clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CPU_RST_CTRL, 1 << cpu);
	udelay(10);
}

static void set_cpu_poweroff(void)
{
	if ((sunxi_efuse_sram_read(&sid, 0x48U) >> 29) & 1U) {
		set_cpu_down(2);
		set_cpu_down(3);
	}
}

int main(void)
{
	axp_pmu_t pmu;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();

	show_banner();
	if (pmu_axp1530_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: configuration failed\n");
		return -1;
	}

	sunxi_clk_init();

	sunxi_clk_dump();

	set_cpu_poweroff();

	sunxi_i2c_init(&i2c);

	pmu_axp1530_init(&pmu);

	pmu_axp1530_dump(&pmu);

	int set_vol = 1100; /* LPDDR4 1100mv */

	int temp_vol, src_vol = pmu_axp1530_get_vol(&pmu, "dcdc3");
	if (src_vol > set_vol) {
		for (temp_vol = src_vol; temp_vol >= set_vol; temp_vol -= 50) {
			pmu_axp1530_set_vol(&pmu, "dcdc3", temp_vol, 1);
		}
	} else if (src_vol < set_vol) {
		for (temp_vol = src_vol; temp_vol <= set_vol; temp_vol += 50) {
			pmu_axp1530_set_vol(&pmu, "dcdc3", temp_vol, 1);
		}
	}

	mdelay(30); /* Delay 300ms for pmu bootup */

	pmu_axp1530_dump(&pmu);

	uint32_t dram_size = sunxi_dram_init(&dram);
	if (dram_size == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	pr_info("DRAM: DRAM Size = %dMB\n", dram_size);

	sunxi_clk_dump();

	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
