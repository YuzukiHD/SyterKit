/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/spi-dt.h>
#include <dt-compatible/spi-nand-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_spi_t spi0 = {0};
	sunxi_spi_t spi1 = {0};
	spi_nand_t nand0 = {0};
	spi_nand_t nand1 = {0};
	spi_nand_t rejected = {.max_frequency = 0xdeadbeefU};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi0, "spi0", NULL));
	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi1, "spi1", NULL));
	TEST_ASSERT(spi0.dt_node != spi1.dt_node);

	TEST_EQ(DRIVER_OK,
		 spi_nand_dt_read_alias(&nand0, "nand0", &spi0));
	TEST_EQ(0U, nand0.chip_select);
	TEST_EQ(24000000U, nand0.max_frequency);
	TEST_ASSERT(nand0.spi == &spi0);
	TEST_EQ(DRIVER_OK,
		 spi_nand_dt_read_alias(&nand1, "nand1", &spi0));
	TEST_EQ(1U, nand1.chip_select);
	TEST_EQ(50000000U, nand1.max_frequency);
	TEST_ASSERT(nand0.dt_node != nand1.dt_node);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "nand0", &spi1));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "nand-disabled", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "nand-bad-cs", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "nand-bad-frequency", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "nand-wrong-compatible", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, "missing", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(NULL, "nand0", &spi0));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 spi_nand_dt_read_alias(&rejected, NULL, &spi0));
	TEST_EQ(0xdeadbeefU, rejected.max_frequency);
}
