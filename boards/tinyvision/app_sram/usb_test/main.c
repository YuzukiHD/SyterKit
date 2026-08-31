/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <mmu.h>

#include <drivers/intc/gic.h>
#include <drivers/usb/usb.h>

void arm32_do_irq(struct arm_regs_t *regs)
{
	do_irq(regs);
}

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_gic_startup() != DRIVER_OK) {
		pr_err("GIC: init failed\n");
		return -1;
	}

	sunxi_clk_init();
	sunxi_usb_attach_module(SUNXI_USB_DEVICE_WINUSB);
	if (sunxi_usb_init() != 0) {
		pr_err("USB: init failed\n");
		return -1;
	}

	pr_info("USB: waiting for host\n");
	sunxi_usb_attach();

	return 0;
}
