/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/dram-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_dram_t dram = { 0 };
	axp_pmu_t vdd_sys_pmu = { 0 };
	axp_pmu_t ddr_pmu = { 0 };

	(void)case_dir;

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "dram24"));

	/* Device-tree loading must retain board-supplied power handles. */
	dram.power.vdd_sys = &vdd_sys_pmu;
	dram.power.ddr = &ddr_pmu;
	TEST_EQ(DRIVER_OK, sunxi_dram_dt_read_alias(&dram, "dram32"));
	TEST_ASSERT(dram.power.vdd_sys == &vdd_sys_pmu);
	TEST_ASSERT(dram.power.ddr == &ddr_pmu);
	TEST_EQ(32U, dram.parameter_count);
	TEST_EQ(0x200U, dram.parameters[0]);
	TEST_EQ(0x21fU, dram.parameters[31]);

	/* Every accepted node uses the same fixed 32-word container. */
	TEST_EQ(DRIVER_OK, sunxi_dram_dt_read_alias(&dram, "dram32-default"));
	TEST_EQ(32U, dram.parameter_count);
	TEST_EQ(0x300U, dram.parameters[0]);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "dram-bad-size"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "dram-bad-count"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "dram-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "dram-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_dram_dt_read_alias(&dram, "missing"));

	dram.size = 512U;
	TEST_EQ(512U, sunxi_get_dram_size(&dram));
	TEST_EQ(512U, sunxi_dram_init(&dram));
	TEST_EQ(0U, sunxi_get_dram_size(NULL));
	TEST_EQ(0U, sunxi_dram_init(NULL));
}
