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
		.base = {
			0xdeadbeefU,
			0xcafef00dU,
		},
	};

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_ccu_dt_read_alias(&rejected, alias));
	TEST_EQ(0xdeadbeefU, rejected.base[0]);
	TEST_EQ(0xcafef00dU, rejected.base[1]);
}

void test_case_main(const char *case_dir) {
	sunxi_ccu_t ccu;
	sunxi_ccu_t second;
	sunxi_ccu_t rejected = {.base = {0xdeadbeefU}};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_ccu_dt_read(&ccu));
	TEST_EQ(0x1000U, ccu.base[0]);
	TEST_EQ(0U, ccu.base[1]);
	TEST_EQ(0U, ccu.base[2]);
	TEST_EQ(0U, ccu.base[3]);

	second = read_alias("ccu1");
	TEST_EQ(0x2000U, second.base[0]);

	ccu = read_alias("ccu-sun252");
	TEST_EQ(0x3000U, ccu.base[0]);

	ccu = read_alias("ccu-sun300");
	TEST_EQ(0x4000U, ccu.base[0]);
	TEST_EQ(0x5000U, ccu.base[1]);

	ccu = read_alias("ccu-sun50iw9");
	TEST_EQ(0x6000U, ccu.base[0]);
	TEST_EQ(0x7000U, ccu.base[1]);
	TEST_EQ(0x8000U, ccu.base[2]);

	ccu = read_alias("ccu-sun50iw10");
	TEST_EQ(0x9000U, ccu.base[0]);
	TEST_EQ(0xa000U, ccu.base[1]);
	TEST_EQ(0xb000U, ccu.base[2]);
	TEST_EQ(0xc000U, ccu.base[3]);

	ccu = read_alias("ccu-sun55iw3");
	TEST_EQ(0xd000U, ccu.base[0]);
	TEST_EQ(0xe000U, ccu.base[1]);
	TEST_EQ(0xf000U, ccu.base[2]);
	TEST_EQ(0x10000U, ccu.base[3]);

	ccu = read_alias("ccu-sun55iw6");
	TEST_EQ(0x11000U, ccu.base[0]);
	TEST_EQ(0x12000U, ccu.base[1]);

	ccu = read_alias("ccu-sun60iw2");
	TEST_EQ(0x13000U, ccu.base[0]);
	TEST_EQ(0x14000U, ccu.base[1]);
	TEST_EQ(0x18000U, ccu.base[2]);

	ccu = read_alias("ccu-sun65iw1");
	TEST_EQ(0x19000U, ccu.base[0]);

	ccu = read_alias("ccu-sun8iw20");
	TEST_EQ(0x1a000U, ccu.base[0]);

	ccu = read_alias("ccu-sun8iw21");
	TEST_EQ(0x1b000U, ccu.base[0]);

	ccu = read_alias("ccu-sun8iw22");
	TEST_EQ(0x1c000U, ccu.base[0]);
	TEST_EQ(0x1d000U, ccu.base[1]);

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
	TEST_EQ(0xdeadbeefU, rejected.base[0]);
}
