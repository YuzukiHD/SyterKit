/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <dt-bindings/soc/sun252iw2.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <drivers/dma/dma.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mtd/spi-nand.h>
#include <drivers/mtd/spi-nor.h>
#include <drivers/psram/psram.h>
#include <drivers/pwm/pwm.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/i2c-dt.h>

#include <common.h>
#include <cache.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/psram-dt.h>
#include <dt-compatible/spi-dt.h>
#include <dt-compatible/spi-nor-dt.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	sunxi_clk_init();

	printk_info("Hello World!\n");

	sunxi_clk_dump();

#ifdef CONFIG_DRIVER_PSRAM
	sunxi_psram_t psram = { 0 };

	if (sunxi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK) {
		printk_error("PSRAM: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_psram_init(&psram);

	printk_info("LPSRAM Size = %d MB\n", sunxi_get_psram_size(&psram));
#endif

#ifdef CONFIG_DRIVER_SPI
	sunxi_spi_t spi = { 0 };
	spi_nor_t nor = { 0 };
	sunxi_dma_t dma = { 0 };

	if (sunxi_dma_dt_read_alias(&dma, "dma0") != DRIVER_OK) {
		printk_error("DMA: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_spi_dt_read_alias(&spi, "spi0", &dma) != DRIVER_OK ||
	    spi_nor_dt_read_alias(&nor, "spi-nor0", &spi) != DRIVER_OK) {
		printk_error("SPI: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_spi_init(&spi);

	if (spi_nor_detect(&nor) != 0) {
		printk_error("SPI NOR: no supported flash detected\n");
		return -1;
	}

	/* Read the first 1MiB of the SPI NOR into PSRAM and report throughput. */
	uint32_t nor_read_size = 1024 * 1024;
	uint32_t time_start = time_ms();
	spi_nor_read(&nor, (void *)SUNXI_PSRAM_BASE, 0x0, nor_read_size);
	uint32_t time_end = time_ms();
	uint32_t delta = (time_end - time_start);
	if (delta == 0U)
		delta = 1U;
	printk_info("SPI NOR: read %uKiB in %ums, %uKiB/s\n", nor_read_size / 1024, delta, (nor_read_size / 1024) / delta);
	dump_hex(SUNXI_PSRAM_BASE, 0x40);
#endif

	syterkit_shell_attach(NULL);

	abort();

	return 0;
}
