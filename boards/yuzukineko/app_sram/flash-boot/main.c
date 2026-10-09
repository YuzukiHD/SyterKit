/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Flash boot loader of the Yuzuki Neko (F101 C907): copies a program from the SPI NOR flash
 * into PSRAM and starts it.
 *
 * The BROM loads this SPL into SRAM. It brings up the PSRAM, picks one of two programs and
 * copies it from the SPI NOR flash to the address its header names:
 *
 *	0x000000  this image, three copies at 64 KiB intervals (0x000000, 0x010000, 0x020000)
 *	0x030000  system program: UF2 downloader (header page, then the image)
 *	0x200000  application (header page, then the image; optionally a second part, the "extra")
 *
 * The application is started, unless the key on PD5 is held (to ground) when the board starts
 * or the flash holds no valid application: then the system program is started, which shows
 * the board as a USB drive for .uf2 files.
 *
 * A header page (4 KiB, struct boot_header) comes in front of every image. The "extra" part
 * (the sketch of the Arduino core) is a program of its own at a flash offset the header names,
 * starting with struct extra_header; it is copied to extra_load.
 */

#include <stdint.h>

#include <cache.h>
#include <common.h>
#include <driver.h>
#include <log.h>
#include <malloc.h>
#include <timer.h>
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

#define NOR_SYS_HEADER_OFFSET 0x00030000U
#define NOR_SYS_MAX_SIZE      0x001CF000U
#define NOR_APP_HEADER_OFFSET 0x00200000U
#define NOR_APP_MAX_SIZE      0x004CF000U
#define NOR_HEADER_SIZE	      0x00001000U
#define NOR_EXTRA_MAX_SIZE    0x00200000U

#define BOOT_MAGIC   0x44524146U /* "FARD" */
#define BOOT_VERSION 2U

/* the key: PD5, pulled up, held to ground */
#define KEY_PORT GPIO_PORTD
#define KEY_PIN	 5

/* the PSRAM above the core region is free while loading: the heap of the SPIF training lives there */
#define HEAP_BASE (SUNXI_PSRAM_BASE + 0x00800000U)
#define HEAP_SIZE 0x00500000U

/* first words of the extra part (for the Arduino core: the sketch header, cores/f101/sketch_abi.h) */
#define EXTRA_MAGIC 0x31303146U /* "F101" */
struct extra_header {
	uint32_t magic;
	uint32_t abi;
	uint32_t core_id;
	uint32_t fn[4];
	uint32_t init_array[2];
	uint32_t bss[2];
	uint32_t image_end;
};

struct boot_header {
	uint32_t magic;
	uint32_t version;
	uint32_t load;	     /* PSRAM address of the image, also its entry */
	uint32_t size;	     /* bytes of the image that follow the header page */
	uint32_t id;
	uint32_t extra_load; /* PSRAM address of the extra part, 0: none */
	uint32_t extra_max;
	uint32_t extra_offset; /* flash offset of the extra part */
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

static bool key_held(void)
{
	gpio_mux_t key = {
		.base = SUNXI_GPIO_BASE,
		.pin = GPIO_PIN(KEY_PORT, KEY_PIN),
		.bank = KEY_PORT - GPIO_PORTA,
		.mux = GPIO_INPUT,
	};

	sunxi_gpio_init(&key);
	sunxi_gpio_set_pull(&key, GPIO_PULL_UP);
	mdelay(5);
	return sunxi_gpio_read(&key) == 0;
}

static bool header_ok(const struct boot_header *h, uint32_t max_size)
{
	return h->magic == BOOT_MAGIC && h->version == BOOT_VERSION && h->size != 0U &&
	       h->size <= max_size && h->load >= SUNXI_PSRAM_BASE &&
	       (h->extra_load == 0U || h->load + h->size <= h->extra_load);
}

int main(void)
{
	sunxi_spif_t spif = { 0 };
	spif_nor_t nor = { 0 };
	struct boot_header hdr, app;
	struct extra_header ex;
	bool app_ok, key;
	uint32_t offset, max_size;

	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	show_banner();
	sunxi_clk_init();

	key = key_held();
	if (psram_up())
		return -1;
	if (malloc_init(HEAP_BASE, HEAP_SIZE) != 0)
		pr_warn("SPI NOR: heap init failed, sampling training disabled\n");
	if (nor_up(&spif, &nor))
		return -1;

	app_ok = spif_nor_read(&nor, (uint8_t *)&app, NOR_APP_HEADER_OFFSET, sizeof(app)) == sizeof(app) &&
		 header_ok(&app, NOR_APP_MAX_SIZE);
	if (key || !app_ok) {
		pr_info("boot: %s, starting the system program\n", key ? "key held" : "no valid application");
		if (spif_nor_read(&nor, (uint8_t *)&hdr, NOR_SYS_HEADER_OFFSET, sizeof(hdr)) != sizeof(hdr) ||
		    !header_ok(&hdr, NOR_SYS_MAX_SIZE)) {
			pr_err("flash: no valid system program (magic 0x%08x)\n", hdr.magic);
			return -1;
		}
		offset = NOR_SYS_HEADER_OFFSET;
		max_size = NOR_SYS_MAX_SIZE;
	} else {
		hdr = app;
		offset = NOR_APP_HEADER_OFFSET;
		max_size = NOR_APP_MAX_SIZE;
	}
	(void)max_size;

	if (nor_read(&nor, "image", offset + NOR_HEADER_SIZE, hdr.load, hdr.size))
		return -1;

	/* the extra part is optional: the program reports that none is loaded */
	if (hdr.extra_load != 0U) {
		if (spif_nor_read(&nor, (uint8_t *)&ex, hdr.extra_offset, sizeof(ex)) == sizeof(ex) &&
		    ex.magic == EXTRA_MAGIC && ex.image_end > hdr.extra_load &&
		    ex.image_end - hdr.extra_load <= NOR_EXTRA_MAX_SIZE) {
			if (nor_read(&nor, "extra", hdr.extra_offset, hdr.extra_load,
				     ex.image_end - hdr.extra_load))
				return -1;
		} else {
			pr_warn("flash: no extra part\n");
		}
	}

	pr_info("starting at 0x%08x (id %u)\n", hdr.load, hdr.id);
	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");
	((void (*)(void))(uintptr_t)hdr.load)();
	__builtin_unreachable();
}
