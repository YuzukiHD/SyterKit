/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/usb-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_usb_t usb0 = { 0 };
	sunxi_usb_t usb1 = { 0 };
	sunxi_usb_t rejected = { .base = 0xdeadbeefU };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_usb_dt_read_alias(&usb0, "usb0"));
	TEST_EQ(0x1000U, usb0.base);
	TEST_EQ(0U, usb0.id);
	TEST_EQ(61U, usb0.irq);
	TEST_EQ(0x4000U, usb0.phy_clock_reg_base);
	TEST_EQ(31U, usb0.phy_clock_gate_offset);
	TEST_EQ(30U, usb0.phy_reset_offset);
	TEST_EQ(0x4100U, usb0.clock_gate_reg_base);
	TEST_EQ(8U, usb0.clock_gate_offset);
	TEST_EQ(24U, usb0.reset_offset);
	TEST_ASSERT(!usb0.detected);

	TEST_EQ(DRIVER_OK, sunxi_usb_dt_read_alias(&usb1, "usb1"));
	TEST_EQ(0x2000U, usb1.base);
	TEST_EQ(1U, usb1.id);
	TEST_EQ(62U, usb1.irq);
	TEST_EQ(0x4200U, usb1.phy_clock_reg_base);
	TEST_EQ(4U, usb1.phy_clock_gate_offset);
	TEST_EQ(5U, usb1.phy_reset_offset);
	TEST_EQ(0x4300U, usb1.clock_gate_reg_base);
	TEST_EQ(6U, usb1.clock_gate_offset);
	TEST_EQ(7U, usb1.reset_offset);
	TEST_ASSERT(usb0.dt_node != usb1.dt_node);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "usb-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "usb-invalid-id"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "usb-invalid-offset"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "usb-invalid-reset"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "usb-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_alias(NULL, "usb0"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_usb_dt_read_config(&rejected, -1));
	TEST_EQ(0xdeadbeefU, rejected.base);
}
