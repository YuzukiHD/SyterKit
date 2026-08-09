/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-compatible/ccu-dt.h>

#include "syter_test.h"

static sunxi_ccu_t read_alias(const char *alias) {
	sunxi_ccu_t ccu = {0};

	TEST_EQ(DRIVER_OK, sunxi_ccu_dt_read_alias(&ccu, alias));
	return ccu;
}

static void test_rejected_alias(const char *alias) {
	sunxi_ccu_t rejected = {
		.base = 0xdeadbeefU,
		.aon_base = 0xcafef00dU,
	};

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_ccu_dt_read_alias(&rejected, alias));
	TEST_EQ(0xdeadbeefU, rejected.base);
	TEST_EQ(0xcafef00dU, rejected.aon_base);
}

void test_case_main(const char *case_dir) {
	sunxi_ccu_t ccu;
	sunxi_ccu_t second;
	sunxi_ccu_t rejected = {.base = 0xdeadbeefU};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_ccu_dt_read(&ccu));
	TEST_EQ(0x1000U, ccu.base);
	TEST_EQ(0xd04U, ccu.size);
	TEST_EQ(0U, ccu.aon_base);
	TEST_EQ(0U, ccu.cpu_pll_base);

	second = read_alias("ccu1");
	TEST_EQ(0x2000U, second.base);
	TEST_EQ(0xd04U, second.size);
	TEST_ASSERT(ccu.dt_node != second.dt_node);

	ccu = read_alias("ccu-sun252");
	TEST_EQ(0x3000U, ccu.base);
	TEST_EQ(0xd04U, ccu.size);

	ccu = read_alias("ccu-sun300");
	TEST_EQ(0x4000U, ccu.base);
	TEST_EQ(0x100U, ccu.size);
	TEST_EQ(0x5000U, ccu.aon_base);
	TEST_EQ(0x58cU, ccu.aon_size);

	ccu = read_alias("ccu-sun50iw9");
	TEST_EQ(0x6000U, ccu.base);
	TEST_EQ(0x9f0U, ccu.size);
	TEST_EQ(0x7000U, ccu.r_prcm_base);
	TEST_EQ(0x314U, ccu.r_prcm_size);
	TEST_EQ(0x8000U, ccu.iommu_base);
	TEST_EQ(0x44U, ccu.iommu_size);

	ccu = read_alias("ccu-sun50iw10");
	TEST_EQ(0x9000U, ccu.base);
	TEST_EQ(0x7c0U, ccu.size);
	TEST_EQ(0xa000U, ccu.r_prcm_base);
	TEST_EQ(0x258U, ccu.r_prcm_size);
	TEST_EQ(0xb000U, ccu.sysctrl_base);
	TEST_EQ(0x164U, ccu.sysctrl_size);
	TEST_EQ(0xc000U, ccu.iommu_base);
	TEST_EQ(0x44U, ccu.iommu_size);

	ccu = read_alias("ccu-sun55iw3");
	TEST_EQ(0xd000U, ccu.base);
	TEST_EQ(0x7c0U, ccu.size);
	TEST_EQ(0xe000U, ccu.cpu_sys_cfg_base);
	TEST_EQ(0x70U, ccu.cpu_sys_cfg_size);
	TEST_EQ(0xf000U, ccu.r_prcm_base);
	TEST_EQ(0x314U, ccu.r_prcm_size);
	TEST_EQ(0x10000U, ccu.iommu_base);
	TEST_EQ(0x44U, ccu.iommu_size);

	ccu = read_alias("ccu-sun55iw6");
	TEST_EQ(0x11000U, ccu.base);
	TEST_EQ(0x58cU, ccu.size);
	TEST_EQ(0x12000U, ccu.cpu_pll_base);
	TEST_EQ(0x50U, ccu.cpu_pll_size);

	ccu = read_alias("ccu-sun60iw2");
	TEST_EQ(0x13000U, ccu.base);
	TEST_EQ(0x584U, ccu.size);
	TEST_EQ(0x14000U, ccu.cpu_pll_base);
	TEST_EQ(0x3020U, ccu.cpu_pll_size);
	TEST_EQ(0x18000U, ccu.rtc_base);
	TEST_EQ(0x164U, ccu.rtc_size);

	ccu = read_alias("ccu-sun65iw1");
	TEST_EQ(0x19000U, ccu.base);
	TEST_EQ(0xa4U, ccu.size);

	ccu = read_alias("ccu-sun8iw20");
	TEST_EQ(0x1a000U, ccu.base);
	TEST_EQ(0xa90U, ccu.size);

	ccu = read_alias("ccu-sun8iw21");
	TEST_EQ(0x1b000U, ccu.base);
	TEST_EQ(0xa90U, ccu.size);

	ccu = read_alias("ccu-sun8iw22");
	TEST_EQ(0x1c000U, ccu.base);
	TEST_EQ(0x344U, ccu.size);
	TEST_EQ(0x1d000U, ccu.cpu_pll_base);
	TEST_EQ(0x24U, ccu.cpu_pll_size);

	test_rejected_alias("ccu-disabled");
	test_rejected_alias("ccu-missing-resource");
	test_rejected_alias("ccu-short-main");
	test_rejected_alias("ccu-short-aux");
	test_rejected_alias("ccu-unaligned");
	test_rejected_alias("ccu-overflow");
	test_rejected_alias("ccu-wrong");
	test_rejected_alias("missing");
	test_rejected_alias(NULL);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_ccu_dt_read_config(&rejected, -1));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_ccu_dt_read_alias(NULL, "ccu0"));
	TEST_EQ(0xdeadbeefU, rejected.base);
}
