/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/rtc-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_rtc_t rtc = { 0 };
	sunxi_rtc_t rejected = { .data_base = 0xdeadbeefU };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_rtc_dt_read_alias(&rtc, "rtc0"));
	TEST_EQ(0x1000U, rtc.data_base);
	TEST_EQ(0x20U, rtc.data_size);
	TEST_ASSERT(rtc.dt_node >= 0);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, "rtc-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, "rtc-short"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, "rtc-unaligned"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, "rtc-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_rtc_dt_read_alias(NULL, "rtc0"));
	TEST_EQ(0xdeadbeefU, rejected.data_base);
}
