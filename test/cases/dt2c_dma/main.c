/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/dma-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_dma_t dma0 = { 0 };
	sunxi_dma_t dma1 = { 0 };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_dma_dt_read_alias(&dma0, "dma0"));
	TEST_EQ(0x7000U, dma0.dma_reg_base);
	TEST_EQ(0x8000U, dma0.bus_clk.gate_reg_base);
	TEST_EQ(2U, dma0.bus_clk.gate_reg_offset);
	TEST_EQ(0x8100U, dma0.dma_clk.gate_reg_base);
	TEST_EQ(3U, dma0.dma_clk.gate_reg_offset);
	TEST_EQ(0x8200U, dma0.dma_clk.rst_reg_base);
	TEST_EQ(19U, dma0.dma_clk.rst_reg_offset);
	TEST_ASSERT(!dma0.initialized);

	TEST_EQ(DRIVER_OK, sunxi_dma_dt_read_alias(&dma1, "dma1"));
	TEST_EQ(0x9000U, dma1.dma_reg_base);
	TEST_EQ(0xa000U, dma1.bus_clk.gate_reg_base);
	TEST_EQ(4U, dma1.bus_clk.gate_reg_offset);
	TEST_EQ(0xa100U, dma1.dma_clk.gate_reg_base);
	TEST_EQ(5U, dma1.dma_clk.gate_reg_offset);
	TEST_EQ(0xa200U, dma1.dma_clk.rst_reg_base);
	TEST_EQ(21U, dma1.dma_clk.rst_reg_offset);
	TEST_ASSERT(!dma1.initialized);
}
