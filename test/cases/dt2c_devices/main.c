/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>
#include <dt-compatible/serial-dt.h>
#include <dt-compatible/spi-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_dma_t dma0 = {0};
	sunxi_dma_t dma1 = {0};
	sunxi_i2c_t i2c0 = {0};
	sunxi_i2c_t i2c1 = {0};
	sunxi_serial_t uart;
	axp_pmu_t pmu0 = {0};
	axp_pmu_t pmu1 = {0};
	sunxi_spi_t spi0 = {0};
	sunxi_spi_t spi4 = {0};
	int disabled_uart;
	int stdout_uart;

	(void) case_dir;
	stdout_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE,
					   "/soc/serial@2000");
	disabled_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE,
					     "/disabled-bus/serial@1000");
	TEST_ASSERT(stdout_uart >= 0);
	TEST_ASSERT(disabled_uart >= 0);
	TEST_EQ(stdout_uart, sunxi_serial_dt_stdout_node());
	TEST_ASSERT(sunxi_serial_dt_node_available(stdout_uart));
	TEST_ASSERT(!sunxi_serial_dt_node_available(disabled_uart));

	TEST_EQ(DRIVER_OK, sunxi_serial_dt_read_config(&uart));
	TEST_EQ(0x2000U, uart.base);
	TEST_EQ(1U, uart.id);
	TEST_EQ(1500000U, uart.baud_rate);
	TEST_EQ(24000000U, uart.uart_clk.parent_clk);
	TEST_EQ(0x3000U, uart.uart_clk.gate_reg_base);
	TEST_EQ(1U, uart.uart_clk.gate_reg_offset);
	TEST_EQ(0x3000U, uart.uart_clk.rst_reg_base);
	TEST_EQ(17U, uart.uart_clk.rst_reg_offset);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 13), uart.gpio_pin.gpio_tx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, uart.gpio_pin.gpio_tx.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 14), uart.gpio_pin.gpio_rx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, uart.gpio_pin.gpio_rx.mux);
	TEST_EQ(UART_DLEN_7, uart.dlen);
	TEST_EQ(UART_STOP_BIT_1, uart.stop);
	TEST_EQ(UART_PARITY_EVEN, uart.parity);

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

	TEST_EQ(DRIVER_OK, sunxi_i2c_dt_read_alias(&i2c0, "i2c0"));
	TEST_EQ(0x3000U, i2c0.base);
	TEST_EQ(SUNXI_R_I2C0, i2c0.id);
	TEST_EQ(400000U, i2c0.speed);
	TEST_EQ(24000000U, i2c0.i2c_clk.parent_clk);
	TEST_EQ(0x4000U, i2c0.i2c_clk.gate_reg_base);
	TEST_EQ(0U, i2c0.i2c_clk.gate_reg_offset);
	TEST_EQ(0x4000U, i2c0.i2c_clk.rst_reg_base);
	TEST_EQ(16U, i2c0.i2c_clk.rst_reg_offset);
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 0), i2c0.gpio.gpio_scl.pin);
	TEST_EQ(GPIO_PERIPH_MUX2, i2c0.gpio.gpio_scl.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 1), i2c0.gpio.gpio_sda.pin);
	TEST_EQ(GPIO_PERIPH_MUX2, i2c0.gpio.gpio_sda.mux);
	TEST_ASSERT(!i2c0.status);

	TEST_EQ(DRIVER_OK, sunxi_i2c_dt_read_alias(&i2c1, "i2c1"));
	TEST_EQ(0x5000U, i2c1.base);
	TEST_EQ(SUNXI_I2C1, i2c1.id);
	TEST_EQ(100000U, i2c1.speed);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu0", &i2c1));
	TEST_EQ(DRIVER_OK,
		 sunxi_pmu_dt_read_alias(&pmu0, "pmu0", &i2c0));
	TEST_ASSERT(pmu0.i2c == &i2c0);
	TEST_EQ(AXP_PMU_AXP2202, pmu0.type);
	TEST_EQ(0x34U, pmu0.address);
	TEST_EQ(0x35U, pmu0.fallback_address);

	TEST_EQ(DRIVER_OK,
		 sunxi_pmu_dt_read_alias(&pmu1, "pmu1", &i2c1));
	TEST_ASSERT(pmu1.i2c == &i2c1);
	TEST_EQ(AXP_PMU_AXP1530, pmu1.type);
	TEST_EQ(0x36U, pmu1.address);
	TEST_EQ(0U, pmu1.fallback_address);
}
