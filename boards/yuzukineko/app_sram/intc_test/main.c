/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <common.h>
#include <driver.h>
#include <drivers/intc/plic.h>
#include <log.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != DRIVER_OK)
		return -1;

	show_banner();
	if (sunxi_plic_startup() != DRIVER_OK) {
		pr_err("PLIC: init failed\n");
		return -1;
	}
	pr_info("PLIC: initialized; USB OTG source is 29\n");
	for (;;)
		__asm__ __volatile__("wfi");
}
