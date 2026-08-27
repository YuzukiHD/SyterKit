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

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>
#include <dt-compatible/psram-dt.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	sunxi_clk_init();

	printk_info("Hello World!\n");

	sunxi_clk_dump();

#ifdef CONFIG_DRIVER_PSRAM
	/* The prebuilt libpsram.a is ELF32-only; it cannot link into the RV64
	 * build.  Skip PSRAM init until a 64-bit library is available. */
	sunxi_psram_t psram = { 0 };

	if (sunxi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK) {
		printk_error("PSRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_psram_init(&psram);

	printk_info("LPSRAM Size = %d MB\n", sunxi_get_psram_size(&psram));
#endif

	syterkit_shell_attach(NULL);

	abort();

	return 0;
}
