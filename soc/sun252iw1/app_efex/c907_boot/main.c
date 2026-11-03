/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>
#include <io.h>
#include <log.h>
#include <uart.h>
#include <drivers/serial/serial.h>
#ifdef CONFIG_DRIVER_PSRAM
#include <dt-compatible/psram-sqpi-dt.h>
#endif
#include <dt-bindings/soc/sun252iw1.h>

#define C907_RESET_CONFIG 0x504U
#define C907_PIC_CLOCK	  0x508U
#define C907_CONFIG_BUS	  0x50cU
#define C907_CPU_CLOCK	  0x500U
#define C907_CONFIG_BASE  0x08008000U
#define C907_RESET_ENTRY  0x100U
#define C907_CORE_CONFIG  0x108U

#ifndef CONFIG_DRIVER_PSRAM
#ifndef C907_ENTRY
#define C907_ENTRY 0x40000000U
#endif

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

#endif

static void c907_prepare_clock(void)
{
	setbits_le32(SUNXI_CCU_BASE + C907_PIC_CLOCK, 1U << 31);
	setbits_le32(SUNXI_CCU_BASE + C907_CONFIG_BUS, 0x00010001U);
	setbits_le32(SUNXI_CCU_BASE + C907_RESET_CONFIG, 0x16aa100cU);
	asm volatile("fence ow, ow" ::: "memory");
}

static void c907_set_entry(uintptr_t entry)
{
	clrsetbits_le32(C907_CONFIG_BASE + C907_CORE_CONFIG, 0x300U, 0x100U);
	writel((uint32_t)entry, C907_CONFIG_BASE + C907_RESET_ENTRY);
	writel(0U, C907_CONFIG_BASE + C907_RESET_ENTRY + 4U);
	asm volatile("fence ow, ow" ::: "memory");
}

static void c907_release_reset(void)
{
	clrbits_le32(SUNXI_CCU_BASE + C907_CPU_CLOCK, 1U << 17);
	setbits_le32(SUNXI_CCU_BASE + C907_RESET_CONFIG, 0x16aa0103U);
	asm volatile("fence ow, ow" ::: "memory");
	setbits_le32(SUNXI_CCU_BASE + C907_RESET_CONFIG, 0x16aa0010U);
	asm volatile("fence ow, ow" ::: "memory");
	setbits_le32(SUNXI_CCU_BASE + C907_RESET_CONFIG, 0x16aa0020U);
	asm volatile("fence ow, ow" ::: "memory");
}

static void __attribute__((noreturn)) e907_park(void)
{
	asm volatile("csrrc zero, mexstatus, %0" : : "r"(0xcU) : "memory");
	asm volatile("csrrs zero, mexstatus, %0" : : "r"(0x4U) : "memory");
	for (;;)
		asm volatile("wfi");
}

int main(void)
{
	uintptr_t entry;

#ifdef CONFIG_DRIVER_PSRAM
	sunxi_psram_t psram;

	if (sunxi_serial_init_stdout() != DRIVER_OK || sunxi_sqpi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK)
		return -1;
	entry = psram.memory_base;
#else
	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	entry = C907_ENTRY;
#endif
	pr_info("Starting C907 RV32 at %08x\n", (uint32_t)entry);
	sunxi_serial_flush(&uart_dbg);
	asm volatile("csrc mstatus, %0" : : "r"(0x8U) : "memory");
	c907_prepare_clock();
	c907_set_entry(entry);
	c907_release_reset();
	e907_park();
}
