/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Boot path of the Arduino core for Yuzuki Neko (F101 C907) from SPI NOR.
 *
 * The BROM loads this SPL into SRAM. It brings up the PSRAM, copies the prebuilt core and the
 * sketch from fixed places of the SPI NOR flash into PSRAM and starts the core.
 *
 * SPI NOR layout (16 MiB flash):
 *	0x000000  this image, three copies at 64 KiB intervals (0x000000, 0x010000, 0x020000)
 *	0x030000  core header page (4 KiB, struct arduino_boot_header), then the core image
 *	0x500000  sketch image (its first bytes are the sketch header, which tells the size)
 *
 * PSRAM layout: the core is linked at 0x40000000, the sketch runs from the window at 0x40e00000.
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

#define NOR_CORE_HEADER_OFFSET 0x00030000U
#define NOR_CORE_HEADER_SIZE   0x00001000U
#define NOR_CORE_OFFSET	       (NOR_CORE_HEADER_OFFSET + NOR_CORE_HEADER_SIZE)
#define NOR_CORE_MAX_SIZE      0x004CF000U
#define NOR_SKETCH_OFFSET      0x00500000U
#define NOR_SKETCH_MAX_SIZE    0x00200000U

#define BOOT_MAGIC   0x44524146U /* "FARD" */
#define BOOT_VERSION 1U

/* the PSRAM above the core region is free while loading: the heap of the SPIF training lives there */
#define HEAP_BASE (SUNXI_PSRAM_BASE + 0x00800000U)
#define HEAP_SIZE 0x00500000U

/* same as the sketch header of the core (cores/f101/sketch_abi.h): only the first fields are used */
#define SKETCH_MAGIC 0x31303146U /* "F101" */
struct sketch_header {
	uint32_t magic;
	uint32_t abi;
	uint32_t core_id;
	uint32_t fn[4];
	uint32_t init_array[2];
	uint32_t bss[2];
	uint32_t image_end;
};

struct arduino_boot_header {
	uint32_t magic;
	uint32_t version;
	uint32_t core_load;   /* PSRAM address of the core image, also its entry */
	uint32_t core_size;   /* bytes of the core image that follow the header page */
	uint32_t core_id;
	uint32_t sketch_load; /* PSRAM address of the sketch window */
	uint32_t sketch_max;
	uint32_t reserved;
};

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

static int nor_read(spif_nor_t *nor, const char *name, uint32_t offset, uintptr_t dest, uint32_t size)
{
	const uint32_t chunk = 0x10000U;
	uint32_t done = 0;

	while (done < size) {
		uint32_t n = size - done < chunk ? size - done : chunk;

		if (spif_nor_read(nor, (uint8_t *)(dest + done), offset + done, n) != n) {
			pr_err("SPI NOR: %s: short read at 0x%08x\n", name, offset + done);
			return -1;
		}
		done += n;
	}
	pr_info("SPI NOR: %-6s 0x%08x -> 0x%08x (%u KiB)\n", name, offset, (unsigned int)dest, size >> 10);
	return 0;
}

int main(void)
{
	sunxi_spif_t spif = { 0 };
	spif_nor_t nor = { 0 };
	struct arduino_boot_header hdr;
	struct sketch_header sk;

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

	if (spif_nor_read(&nor, (uint8_t *)&hdr, NOR_CORE_HEADER_OFFSET, sizeof(hdr)) != sizeof(hdr)) {
		pr_err("SPI NOR: cannot read the core header\n");
		return -1;
	}
	if (hdr.magic != BOOT_MAGIC || hdr.version != BOOT_VERSION || hdr.core_size == 0U ||
	    hdr.core_size > NOR_CORE_MAX_SIZE || hdr.core_load < SUNXI_PSRAM_BASE ||
	    hdr.core_load + hdr.core_size > hdr.sketch_load) {
		pr_err("flash: no valid core (magic 0x%08x)\n", hdr.magic);
		return -1;
	}

	if (nor_read(&nor, "core", NOR_CORE_OFFSET, hdr.core_load, hdr.core_size))
		return -1;

	/* the sketch is optional: the core reports that none is loaded */
	if (spif_nor_read(&nor, (uint8_t *)&sk, NOR_SKETCH_OFFSET, sizeof(sk)) == sizeof(sk) &&
	    sk.magic == SKETCH_MAGIC && sk.image_end > hdr.sketch_load &&
	    sk.image_end - hdr.sketch_load <= NOR_SKETCH_MAX_SIZE) {
		if (nor_read(&nor, "sketch", NOR_SKETCH_OFFSET, hdr.sketch_load, sk.image_end - hdr.sketch_load))
			return -1;
	} else {
		pr_warn("flash: no sketch\n");
	}

	pr_info("starting the core at 0x%08x (id %u)\n", hdr.core_load, hdr.core_id);
	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");
	((void (*)(void))(uintptr_t)hdr.core_load)();
	__builtin_unreachable();
}
