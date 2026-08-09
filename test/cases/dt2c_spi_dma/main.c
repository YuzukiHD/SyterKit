/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/spi-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_dma_t dma0 = {0};
	sunxi_dma_t dma1 = {0};
	sunxi_spi_t spi0 = {0};
	sunxi_spi_t spi4 = {0};

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_dma_dt_read_alias(&dma0, "dma0"));
	TEST_EQ(DRIVER_OK, sunxi_dma_dt_read_alias(&dma1, "dma1"));

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_spi_dt_read_alias(&spi0, "spi0", &dma0));
	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi0, "spi0", &dma1));
	TEST_EQ(0xb000U, spi0.base);
	TEST_EQ(0U, spi0.id);
	TEST_EQ(75000000U, spi0.clk_rate);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 1), spi0.gpio.gpio_cs.pin);
	TEST_EQ(GPIO_PERIPH_MUX4, spi0.gpio.gpio_cs.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 0), spi0.gpio.gpio_sck.pin);
	TEST_EQ(GPIO_PERIPH_MUX4, spi0.gpio.gpio_sck.mux);
	TEST_ASSERT(spi0.dma_handle == &dma1);
	TEST_EQ(27U, spi0.dma_rx_drq);
	TEST_EQ(0xc000U, spi0.spi_clk.spi_clock_cfg_base);
	TEST_EQ(1U, spi0.spi_clk.spi_clock_source);
	TEST_EQ(8U, spi0.spi_clk.spi_clock_factor_n_offset);
	TEST_EQ(300000000U, spi0.parent_clk_reg.parent_clk);
	TEST_EQ(SPI_CDR1_MODE, spi0.spi_clk.cdr_mode);

	TEST_EQ(DRIVER_OK, sunxi_spi_dt_read_alias(&spi4, "spi4", &dma0));
	TEST_EQ(0xd000U, spi4.base);
	TEST_EQ(4U, spi4.id);
	TEST_EQ(50000000U, spi4.clk_rate);
	TEST_EQ(GPIO_PIN(GPIO_PORTD, 1), spi4.gpio.gpio_cs.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, spi4.gpio.gpio_cs.mux);
	TEST_ASSERT(spi4.dma_handle == &dma0);
	TEST_EQ(31U, spi4.dma_rx_drq);
	TEST_EQ(0xe000U, spi4.spi_clk.spi_clock_cfg_base);
	TEST_EQ(2U, spi4.spi_clk.spi_clock_source);
	TEST_EQ(16U, spi4.spi_clk.spi_clock_factor_n_offset);
	TEST_EQ(200000000U, spi4.parent_clk_reg.parent_clk);
	TEST_EQ(SPI_CDR2_MODE, spi4.spi_clk.cdr_mode);
}
