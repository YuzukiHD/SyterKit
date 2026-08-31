/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/plic-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_plic_t plic = { 0 };
	sunxi_plic_t rejected = { .base = 0xdeadbeefU };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_plic_dt_read_alias(&plic, "intc0"));
	TEST_EQ(0x10000000U, plic.base);
	TEST_EQ(0x201000U, plic.size);
	TEST_EQ(145U, plic.irq_count);
	TEST_ASSERT(plic.dt_node >= 0);
	TEST_ASSERT(!plic.initialized);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, "intc-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, "intc-short"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, "intc-too-many-irqs"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, "intc-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_plic_dt_read_alias(NULL, "intc0"));
	TEST_EQ(0xdeadbeefU, rejected.base);
}
