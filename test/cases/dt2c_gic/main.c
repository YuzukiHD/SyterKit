/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/gic-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_gic_t gic = { 0 };
	sunxi_gic_t rejected = { .distributor_base = 0xdeadbeefU };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_gic_dt_read_alias(&gic, "intc0"));
	TEST_EQ(0x1000U, gic.distributor_base);
	TEST_EQ(0x1000U, gic.distributor_size);
	TEST_EQ(0x3000U, gic.cpu_interface_base);
	TEST_EQ(0x2000U, gic.cpu_interface_size);
	TEST_EQ(223U, gic.irq_count);
	TEST_ASSERT(!gic.initialized);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-short-dist"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-short-cpu"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-too-few-irqs"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-too-many-irqs"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "intc-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_alias(NULL, "intc0"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_gic_dt_read_config(&rejected, -1));
	TEST_EQ(0xdeadbeefU, rejected.distributor_base);
}
