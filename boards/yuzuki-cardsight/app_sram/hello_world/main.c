/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <common.h>
#include <log.h>
#include <drivers/serial/serial.h>
#include <cli/cli_shell.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	pr_info("SRAM boot OK: UART0 PH13 TX / PH14 RX, 115200 8N1.\n");
	pr_info("PSRAM initialization is pending.\n");

	return syterkit_shell_attach(NULL);
}
