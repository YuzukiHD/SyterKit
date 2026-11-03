/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/psram-sqpi-dt.h>
#include <cli/cli_shell.h>

int main(void)
{
	sunxi_psram_t psram = { 0 };

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
#ifdef CONFIG_ARCH_CPU_C907
	sunxi_clk_preinit();
	sunxi_clk_init();
	sunxi_clk_dump();
#endif
	pr_info("SRAM boot OK: UART0 PH13 TX / PH14 RX, 115200 8N1.\n");
	if (sunxi_sqpi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK || !sunxi_psram_init(&psram))
		pr_err("PSRAM initialization failed\n");

	return syterkit_shell_attach(NULL);
}
