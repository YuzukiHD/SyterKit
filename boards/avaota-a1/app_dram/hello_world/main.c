/* SPDX-License-Identifier: GPL-2.0+ */

#include <log.h>
#include <drivers/serial/serial.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;
	pr_info("SyterKit DRAM application is running\n");
	return 0;
}
