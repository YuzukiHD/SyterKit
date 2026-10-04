/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Boot stub for an image that runs from the flash window (XIP) of the SPIF controller, for the
 * Yuzuki Neko / F101 EVB.
 *
 * The BROM (or the FEL loader) starts this SPL in SRAM. It brings up the PSRAM (the RAM of the image),
 * the SPIF controller and the flash with the usual detection and sampling training, maps the flash
 * from XIP_FLASH_OFFSET at the window (0x0e000000) and jumps to the first word of the window, the
 * entry of the image.
 *
 * Build with -DXIP_FLASH_OFFSET=0x... to place the image elsewhere (4 KiB aligned).
 */

#include <stdint.h>

#include <cache.h>
#include <common.h>
#include <driver.h>
#include <log.h>
#include <malloc.h>
#include <uart.h>

#include <drivers/clk/clk.h>
#include <drivers/mtd/spif-nor.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>
#include <drivers/spif/spif.h>
#include <dt-bindings/soc/sun252iw2.h>
#include <dt-compatible/psram-dt.h>
#include <dt-compatible/spif-dt.h>
#include <dt-compatible/spif-nor-dt.h>

#ifndef XIP_FLASH_OFFSET
#define XIP_FLASH_OFFSET 0x00a00000U
#endif
#define XIP_LENGTH 0x00200000U

/* the PSRAM above the image is free while the SPIF training runs: its heap lives there */
#define HEAP_BASE (SUNXI_PSRAM_BASE + 0x00800000U)
#define HEAP_SIZE 0x00500000U

/* console of the EVB: UART3 on PE8/PE9 */
static const sunxi_serial_t console = {
	.base = SUNXI_UART3_BASE,
	.id = 3,
	.uart_clk = {
		.gate_reg_base = SUNXI_CCU_BASE + 0x90c,
		.gate_reg_offset = 3,
		.rst_reg_base = SUNXI_CCU_BASE + 0x90c,
		.rst_reg_offset = 19,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTE, 8), .bank = GPIO_PORTE - GPIO_PORTA, .mux = 6 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTE, 9), .bank = GPIO_PORTE - GPIO_PORTA, .mux = 6 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static int psram_up(void)
{
	sunxi_psram_t psram = { 0 };

	if (sunxi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK) {
		pr_err("PSRAM: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_psram_init(&psram) == 0U) {
		pr_err("PSRAM: initialization failed\n");
		return -1;
	}
	pr_info("PSRAM: %u MiB initialized\n", sunxi_get_psram_size(&psram));
	return 0;
}

static int nor_up(sunxi_spif_t *spif, spif_nor_t *nor)
{
	if (sunxi_spif_dt_read_alias(spif, "spif0") != DRIVER_OK ||
	    spif_nor_dt_read_alias(nor, "spif-nor0", spif) != DRIVER_OK) {
		pr_err("SPI NOR: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_spif_init(spif) != 0) {
		pr_err("SPIF: controller init failed\n");
		return -1;
	}
	if (spif_nor_detect(nor) != 0) {
		pr_err("SPI NOR: no supported flash detected\n");
		return -1;
	}
	pr_info("SPI NOR: flash id 0x%08x, %u KiB\n", nor->info.id, nor->info.capacity >> 10);
	return 0;
}

int main(void)
{
	sunxi_spif_t spif = { 0 };
	spif_nor_t nor = { 0 };

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	show_banner();
	sunxi_clk_init();

	if (psram_up())
		return -1;
	if (malloc_init(HEAP_BASE, HEAP_SIZE) != 0)
		pr_warn("SPI NOR: heap init failed, sampling training disabled\n");
	if (nor_up(&spif, &nor))
		return -1;

	if (spif_nor_xip_enable(&nor, XIP_FLASH_OFFSET, XIP_LENGTH) != 0) {
		pr_err("SPI NOR: cannot map the flash window\n");
		return -1;
	}
	pr_info("XIP: flash 0x%08x mapped at 0x%08x, first words %08x %08x\n", XIP_FLASH_OFFSET, SUNXI_SPIF_XIP_BASE,
		((volatile uint32_t *)SUNXI_SPIF_XIP_BASE)[0], ((volatile uint32_t *)SUNXI_SPIF_XIP_BASE)[1]);

	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");
	((void (*)(void))(uintptr_t)SUNXI_SPIF_XIP_BASE)();
	__builtin_unreachable();
}
