/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/dram-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_dram_t dram = {0};
	sunxi_dram_t dram_alt = {0};
	sunxi_i2c_t i2c0 = {0};
	axp_pmu_t pmu0 = {0};
	axp_pmu_t pmu1 = {0};
	size_t index;

	(void) case_dir;
	i2c0.dt_node = syterkit_dt_alias_node("i2c0", SUNXI_I2C_COMPATIBLE);
	TEST_ASSERT(i2c0.dt_node >= 0);
	pmu0.i2c = &i2c0;
	pmu0.address = 0x34U;
	pmu0.type = AXP_PMU_AXP2202;
	pmu1.i2c = &i2c0;
	pmu1.address = 0x36U;
	pmu1.type = AXP_PMU_AXP1530;

	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram, "dram24", NULL, NULL));
	TEST_EQ(24U, dram.parameter_count);
	TEST_EQ(0x100U, dram.parameters[0]);
	TEST_EQ(0x117U, dram.parameters[23]);
	TEST_ASSERT(dram.primary_pmu == NULL);
	TEST_ASSERT(dram.secondary_pmu == NULL);
	TEST_EQ(0x10000000U, dram.registers.ccu.base);
	TEST_EQ(0x10002000U, dram.registers.mctl_com.base);
	TEST_EQ(0x10003000U, dram.registers.mctl_phy.base);
	TEST_EQ(0x10004000U, dram.registers.sysctrl.base);
	TEST_EQ(0x10005000U, dram.registers.sid.base);
	TEST_EQ(0x10006000U, dram.registers.r_cpucfg.base);
	TEST_EQ(0x10007000U, dram.registers.r_prcm.base);
	TEST_EQ(0U, dram.registers.aon_ccu.base);
	TEST_EQ(0U, dram.registers.pmu_rtc.base);
	TEST_EQ(0x40000000U, dram.memory_base);
	TEST_EQ(0x20000000U, dram.memory_size);
	TEST_EQ(0U, dram.init_code_base);
	TEST_EQ(0U, dram.init_code_size);
	for (index = 24U; index < SUNXI_DRAM_MAX_PARAM_WORDS; ++index)
		TEST_EQ(0U, dram.parameters[index]);
	TEST_EQ(0U, dram.rtc.data_base);

	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram_alt, "dram24-alt", NULL, NULL));
	TEST_EQ(0x300U, dram_alt.parameters[0]);
	TEST_EQ(0x317U, dram_alt.parameters[23]);
	TEST_EQ(0x11000000U, dram_alt.registers.ccu.base);
	TEST_EQ(0x11007000U, dram_alt.registers.r_prcm.base);
	TEST_EQ(0x80000000U, dram_alt.memory_base);
	TEST_EQ(0x40000000U, dram_alt.memory_size);
	TEST_EQ(0x10000000U, dram.registers.ccu.base);
	TEST_EQ(0x40000000U, dram.memory_base);

	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram, "dram21", NULL, NULL));
	TEST_EQ(0x12000000U, dram.registers.ccu.base);
	TEST_EQ(0x12007000U, dram.registers.r_prcm.base);
	TEST_EQ(0x40000000U, dram.memory_base);
	TEST_EQ(0x20000000U, dram.memory_size);
	TEST_EQ(0x1000U, dram.rtc.data_base);
	TEST_EQ(0x100U, dram.rtc.data_size);

	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram, "dram300", NULL, NULL));
	TEST_EQ(0x20001000U, dram.registers.ccu.base);
	TEST_EQ(0x20002000U, dram.registers.aon_ccu.base);
	TEST_EQ(0x20003000U, dram.registers.mctl_com.base);
	TEST_EQ(0x20004000U, dram.registers.mctl_phy.base);
	TEST_EQ(0x20005000U, dram.registers.sysctrl.base);
	TEST_EQ(0x20006000U, dram.registers.r_prcm.base);
	TEST_EQ(0x20007000U, dram.registers.pmu_rtc.base);
	TEST_EQ(0U, dram.registers.sid.base);
	TEST_EQ(0U, dram.registers.r_cpucfg.base);
	TEST_EQ(0x80000000U, dram.memory_base);
	TEST_EQ(0x40000000U, dram.memory_size);

	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram, "dram50iw9", NULL, NULL));
	TEST_EQ(0x1000U, dram.rtc.data_base);
	TEST_EQ(0x100U, dram.rtc.data_size);
	TEST_EQ(0x00048000U, dram.init_code_base);
	TEST_EQ(0x00008000U, dram.init_code_size);
	TEST_EQ(0U, dram.memory_base);
	TEST_EQ(0U, dram.memory_size);
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram50iw9-short-region", NULL,
					  NULL));

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram32", &pmu1, &pmu0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram32", &pmu0, NULL));
	TEST_EQ(DRIVER_OK,
		 sunxi_dram_dt_read_alias(&dram, "dram32", &pmu0, &pmu1));
	TEST_EQ(32U, dram.parameter_count);
	TEST_EQ(0x200U, dram.parameters[0]);
	TEST_EQ(0x21fU, dram.parameters[31]);
	TEST_ASSERT(dram.primary_pmu == &pmu0);
	TEST_ASSERT(dram.secondary_pmu == &pmu1);
	for (index = 32U; index < SUNXI_DRAM_MAX_PARAM_WORDS; ++index)
		TEST_EQ(0U, dram.parameters[index]);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram24", &pmu0, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram25", NULL, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram32-no-pmu", NULL, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "dram-disabled", NULL, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
			 sunxi_dram_dt_read_alias(&dram, "dram24-short-reg", NULL,
						  NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
			 sunxi_dram_dt_read_alias(&dram, "dram24-no-memory", NULL,
						  NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
			 sunxi_dram_dt_read_alias(&dram, "dram24-non-memory", NULL,
						  NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_dram_dt_read_alias(&dram, "missing", NULL, NULL));
	dram.size = 512U;
	TEST_EQ(512U, sunxi_get_dram_size(&dram));
	TEST_EQ(512U, sunxi_dram_init(&dram));
	TEST_EQ(0U, sunxi_get_dram_size(NULL));
	TEST_EQ(0U, sunxi_dram_init(NULL));
}
