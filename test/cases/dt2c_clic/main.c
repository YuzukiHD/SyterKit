/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/clic-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_clic_t clic = {0};
	sunxi_clic_t rejected = {.base = 0xdeadbeefU};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_clic_dt_read_alias(&clic, "intc0"));
	TEST_EQ(0x1000U, clic.base);
	TEST_EQ(0x2000U, clic.size);
	TEST_EQ(208U, clic.irq_count);
	TEST_ASSERT(clic.dt_node >= 0);
	TEST_ASSERT(!clic.initialized);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, "intc-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, "intc-short"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, "intc-too-many-irqs"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, "intc-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_clic_dt_read_alias(NULL, "intc0"));
	TEST_EQ(0xdeadbeefU, rejected.base);
}
