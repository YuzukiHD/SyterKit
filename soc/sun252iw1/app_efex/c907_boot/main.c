/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Hand the system to the C907 after the DRAM is up.
 *
 * Run from FEL on the E907 once init_dram has returned and the image of the C907 is in DRAM:
 *
 *   xfel write 0x100000 init_dram_efex.bin; xfel exec 0x100000
 *   xfel write 0x40000000 <c907 image>
 *   xfel write 0x100000 c907_boot_efex.bin; xfel exec 0x100000
 *
 * The C907 starts as rv32 at C907_ENTRY (the same register sequence as the arch boot payload of
 * the board), then this core sleeps. Nothing else is touched: clocks and the PMU stay as
 * init_dram left them.
 */

#include <common.h>
#include <io.h>
#include <log.h>
#include <uart.h>
#include <dt-bindings/soc/sun252iw1.h>
#include <drivers/serial/serial.h>

#ifndef C907_ENTRY
#define C907_ENTRY 0x40000000U
#endif

#define CCU_BASE        0x02001000U
#define CCU_RESET_CFG   (CCU_BASE + 0x504)
#define CCU_PIC_CLK     (CCU_BASE + 0x508)
#define CCU_CFG_BUS     (CCU_BASE + 0x50c)
#define CCU_CPU_CLK     (CCU_BASE + 0x500)
#define RISCV_CFG_BASE  0x08008000U
#define C907_CORE_CFG   (RISCV_CFG_BASE + 0x108)
#define C907_RESET_ENT  (RISCV_CFG_BASE + 0x100)

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

static inline void fence_io(void)
{
	__asm__ volatile("fence ow, ow" ::: "memory");
}

int main(void)
{
	uint32_t v;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	pr_info("c907_boot: starting the C907 (rv32) at 0x%08x, this core sleeps afterwards\n",
		C907_ENTRY);
	/* the C907 takes the UART over: let everything we said leave the FIFO first */
	while (!(readl(SUNXI_UART0_BASE + 0x14) & BIT(6))) {
	}

	__asm__ volatile("csrc mstatus, %0" ::"r"(0x8));

	/* clock/reset controls of the C907, peripheral clock for the interrupt controller */
	writel(0xf8216310U, CCU_BASE + 0x20);
	v = readl(CCU_PIC_CLK);
	writel((v & 0x7fffffffU) | 0x80000000U, CCU_PIC_CLK);

	/* configuration bus on, resets released */
	writel(readl(CCU_CFG_BUS) | 0x00010001U, CCU_CFG_BUS);
	writel(readl(CCU_RESET_CFG) | 0x16aa100cU, CCU_RESET_CFG);
	fence_io();

	/* reset mode rv32 and the address fetched after reset */
	writel((readl(C907_CORE_CFG) & ~0x300U) | 0x100U, C907_CORE_CFG);
	fence_io();
	writel(C907_ENTRY, C907_RESET_ENT);
	fence_io();

	/* CPU clock and gates, then core 0 and core 1 out of reset */
	writel(readl(CCU_CPU_CLK) & 0xfffdffffU, CCU_CPU_CLK);
	writel(readl(CCU_RESET_CFG) | 0x16aa0103U, CCU_RESET_CFG);
	fence_io();
	writel(readl(CCU_RESET_CFG) | 0x16aa0010U, CCU_RESET_CFG);
	fence_io();
	writel(readl(CCU_RESET_CFG) | 0x16aa0020U, CCU_RESET_CFG);

	/* light sleep, the C907 owns the system from here */
	__asm__ volatile("csrrc zero, mexstatus, %0" ::"r"(0xc));
	__asm__ volatile("csrrs zero, mexstatus, %0" ::"r"(0x4));
	for (;;) {
		__asm__ volatile("wfi");
	}
}
