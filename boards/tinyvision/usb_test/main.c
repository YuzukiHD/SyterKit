/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <mmu.h>

#include <drivers/intc/gic.h>
#include <drivers/usb/usb.h>
#include <dt-compatible/usb-dt.h>

void arm32_do_irq(struct arm_regs_t *regs)
{
	do_irq(regs);
}

int main(void)
{
	sunxi_usb_t usb;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_usb_dt_read_alias(&usb, "usb0") != DRIVER_OK) {
		printk_error("USB: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	if (sunxi_usb_init(&usb)) {
		printk_error("USB: init failed\n");
		return -1;
	}

	printk_info("USB: waiting for host\n");
	sunxi_usb_attach(&usb);
	printk_info("USB: host detected\n");

	abort();

	return 0;
}
