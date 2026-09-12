/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>
#include <dt-compatible/serial-dt.h>
#include <uart.h>

int sunxi_serial_init_stdout(void)
{
	int result = sunxi_serial_dt_read_stdout(&uart_dbg);

	if (result != DRIVER_OK)
		return result;

	sunxi_serial_init(&uart_dbg);

	/* Flush early logs at the first point where the UART is usable. */
	uart_log_console_ready();
	return DRIVER_OK;
}
