/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <driver.h>
#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/intc/plic.h>
#include <drivers/serial/serial.h>
#include <drivers/usb/usb.h>

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	if (sunxi_plic_startup() != DRIVER_OK) {
		pr_err("PLIC: init failed\n");
		return -1;
	}

	sunxi_clk_init();
	sunxi_usb_attach_module(SUNXI_USB_DEVICE_FEL);
	if (sunxi_usb_init() != 0) {
		pr_err("USB: init failed\n");
		return -1;
	}

	pr_info("USB: waiting for host\n");
	sunxi_usb_attach();

	return 0;
}
