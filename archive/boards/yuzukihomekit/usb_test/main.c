/* SPDX-License-Identifier: GPL-2.0+ */

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

	show_banner();
	if (sunxi_usb_dt_read_alias(&usb, "usb0") != DRIVER_OK) {
		pr_err("USB: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	if (sunxi_usb_init(&usb)) {
		pr_err("USB: init failed\n");
		return -1;
	}

	pr_info("USB: waiting for host\n");
	sunxi_usb_attach(&usb);
	pr_info("USB: host detected\n");

	abort();

	return 0;
}
