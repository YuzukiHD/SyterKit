/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/spi-dt.h>
#include <dt-compatible/spi-nor-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_spi_t spi0 = { 0 };
	sunxi_spi_t spi1 = { 0 };
	spi_nor_t nor0 = { 0 };
	spi_nor_t nor1 = { 0 };
	spi_nor_t rejected = { .max_frequency = 0xdeadbeefU };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi0, "spi0", NULL));
	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi1, "spi1", NULL));
	TEST_ASSERT(spi0.dt_node != spi1.dt_node);

	TEST_EQ(DRIVER_OK, spi_nor_dt_read_alias(&nor0, "nor0", &spi0));
	TEST_EQ(0U, nor0.chip_select);
	TEST_EQ(24000000U, nor0.max_frequency);
	TEST_ASSERT(nor0.spi == &spi0);
	TEST_EQ(DRIVER_OK, spi_nor_dt_read_alias(&nor1, "nor1", &spi0));
	TEST_EQ(1U, nor1.chip_select);
	TEST_EQ(50000000U, nor1.max_frequency);
	TEST_ASSERT(nor0.dt_node != nor1.dt_node);

	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "nor0", &spi1));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "nor-disabled", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "nor-bad-cs", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "nor-bad-frequency", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "nor-wrong-compatible", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, "missing", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(NULL, "nor0", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID, spi_nor_dt_read_alias(&rejected, NULL, &spi0));
	TEST_EQ(0xdeadbeefU, rejected.max_frequency);
}
