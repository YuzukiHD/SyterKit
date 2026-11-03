/* SPDX-License-Identifier: MIT */

#include <log.h>
#include <uart.h>
#include <efex.h>
#include <drivers/clk/clk.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/psram-sqpi-dt.h>

int main(void)
{
	sunxi_psram_t psram = { 0 };
	uint32_t size;

	if (sunxi_serial_init_stdout() != DRIVER_OK)
		return -1;
	pr_info("eFEX: initialize PSRAM\n");
	sunxi_clk_init();
	if (sunxi_sqpi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK)
		return -1;
	size = sunxi_psram_init(&psram);
	efex_param_report_psram(&psram, size);
	if (!size)
		return -1;
	pr_info("PSRAM ready; returning to FEL for image download at %08x\n", (uint32_t)psram.memory_base);
	sunxi_serial_flush(&uart_dbg);
	return 0;
}
