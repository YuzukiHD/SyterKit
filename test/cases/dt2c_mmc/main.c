/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/mmc-dt.h>

#include "syter_test.h"

static uintptr_t mmio_addresses[2];
static uint32_t mmio_values[2];

void printk(int level, const char *format, ...)
{
	(void)level;
	(void)format;
}

uint32_t test_mmio_read32(uintptr_t address)
{
	for (size_t index = 0U; index < 2U; ++index) {
		if (mmio_addresses[index] == address)
			return mmio_values[index];
	}
	return 0U;
}

void test_mmio_write32(uintptr_t address, uint32_t value)
{
	for (size_t index = 0U; index < 2U; ++index) {
		if (mmio_addresses[index] == 0U || mmio_addresses[index] == address) {
			mmio_addresses[index] = address;
			mmio_values[index] = value;
			return;
		}
	}
}

void test_case_main(const char *case_dir)
{
	sunxi_sdhci_t mmc0 = { 0 };
	sunxi_sdhci_t mmc2 = { 0 };

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_sdhci_dt_read_alias(&mmc0, "mmc0"));
	TEST_STREQ("SD Card", mmc0.name);
	TEST_EQ(0x04020000U, mmc0.reg_base);
	TEST_EQ(MMC_CONTROLLER_0, mmc0.id);
	TEST_EQ(MMC_TYPE_SD, mmc0.sdhci_mmc_type);
	TEST_EQ(SMHC_WIDTH_4BIT, mmc0.width);
	TEST_EQ(50000000U, mmc0.max_clk);
	TEST_EQ(GPIO_IO_VOLTAGE_3V3, mmc0.io_voltage_uv);
	TEST_ASSERT(!mmc0.sample_fifo_bypass);
	TEST_EQ(0x70080000U, mmc0.dma_des_addr);
	TEST_EQ(0x100000U, mmc0.dma_des_size);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 2), mmc0.pinctrl.gpio_clk.pin);
	TEST_EQ(GPIO_PERIPH_MUX2, mmc0.pinctrl.gpio_clk.mux);
	TEST_EQ(0x02000000U, mmc0.pinctrl.gpio_clk.base);
	TEST_EQ(5, mmc0.pinctrl.gpio_clk.bank);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 3), mmc0.pinctrl.gpio_cmd.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 1), mmc0.pinctrl.gpio_d0.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 4), mmc0.pinctrl.gpio_d3.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTF, 6), mmc0.pinctrl.gpio_cd.pin);
	TEST_EQ(GPIO_INPUT, mmc0.pinctrl.gpio_cd.mux);
	TEST_EQ(0x02000000U, mmc0.pinctrl.gpio_cd.base);
	TEST_ASSERT(mmc0.pinctrl.has_card_detect);
	TEST_EQ(GPIO_LEVEL_LOW, mmc0.pinctrl.cd_level);
	TEST_EQ(0x0200184cU, mmc0.clk_ctrl.gate_reg_base);
	TEST_EQ(0U, mmc0.clk_ctrl.gate_reg_offset);
	TEST_EQ(0x0200184cU, mmc0.clk_ctrl.rst_reg_base);
	TEST_EQ(16U, mmc0.clk_ctrl.rst_reg_offset);
	TEST_EQ(0x02001830U, mmc0.sdhci_clk.reg_base);
	TEST_EQ(8U, mmc0.sdhci_clk.reg_factor_n_offset);
	TEST_EQ(0U, mmc0.sdhci_clk.reg_factor_m_offset);
	TEST_EQ(1U, mmc0.sdhci_clk.default_clk_sel);
	TEST_EQ(24000000U, mmc0.sdhci_clk.source_rates[0]);
	TEST_EQ(300000000U, mmc0.sdhci_clk.source_rates[1]);
	TEST_EQ(600000000U, mmc0.sdhci_clk.source_rates[2]);
	TEST_EQ(0U, mmc0.sdhci_clk.source_rates[3]);

	TEST_EQ(DRIVER_OK, sunxi_sdhci_dt_read_alias(&mmc2, "mmc2"));
	TEST_STREQ("eMMC", mmc2.name);
	TEST_EQ(0x04022000U, mmc2.reg_base);
	TEST_EQ(MMC_CONTROLLER_2, mmc2.id);
	TEST_EQ(MMC_TYPE_EMMC, mmc2.sdhci_mmc_type);
	TEST_EQ(SMHC_WIDTH_8BIT, mmc2.width);
	TEST_EQ(5200000U, mmc2.max_clk);
	TEST_EQ(GPIO_IO_VOLTAGE_1V8, mmc2.io_voltage_uv);
	TEST_ASSERT(mmc2.sample_fifo_bypass);
	TEST_EQ(0x70880000U, mmc2.dma_des_addr);
	TEST_EQ(0x100000U, mmc2.dma_des_size);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 5), mmc2.pinctrl.gpio_clk.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 10), mmc2.pinctrl.gpio_d0.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 16), mmc2.pinctrl.gpio_d7.pin);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 0), mmc2.pinctrl.gpio_ds.pin);
	TEST_EQ(GPIO_PERIPH_MUX3, mmc2.pinctrl.gpio_ds.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTC, 1), mmc2.pinctrl.gpio_rst.pin);
	TEST_ASSERT(!mmc2.pinctrl.has_card_detect);
	TEST_EQ(2U, mmc2.clk_ctrl.gate_reg_offset);
	TEST_EQ(18U, mmc2.clk_ctrl.rst_reg_offset);
	TEST_EQ(0x02001838U, mmc2.sdhci_clk.reg_base);
	TEST_EQ(12U, mmc2.sdhci_clk.reg_factor_n_offset);
	TEST_EQ(4U, mmc2.sdhci_clk.reg_factor_m_offset);
	TEST_EQ(3U, mmc2.sdhci_clk.default_clk_sel);
	TEST_EQ(26000000U, mmc2.sdhci_clk.source_rates[0]);
	TEST_EQ(720000000U, mmc2.sdhci_clk.source_rates[1]);
	TEST_EQ(540000000U, mmc2.sdhci_clk.source_rates[2]);
	TEST_EQ(810000000U, mmc2.sdhci_clk.source_rates[3]);
	TEST_EQ(24000000U, mmc0.sdhci_clk.source_rates[0]);
	TEST_EQ(300000000U, mmc0.sdhci_clk.source_rates[1]);

	TEST_ASSERT(mmc0.name != mmc2.name);
	TEST_ASSERT(mmc0.dt_node != mmc2.dt_node);
	TEST_ASSERT(&mmc0.mmc != &mmc2.mmc);
	TEST_ASSERT(&mmc0.mmc_host != &mmc2.mmc_host);
	TEST_ASSERT(&mmc0.timing_data != &mmc2.timing_data);
	TEST_EQ(0, sunxi_sdhci_set_mclk(&mmc0, 50000000U));
	TEST_EQ(1U, (test_mmio_read32(mmc0.sdhci_clk.reg_base) >> 24) & 0x3U);
	TEST_EQ(50000000U, sunxi_sdhci_get_mclk(&mmc0));
	TEST_EQ(0, sunxi_sdhci_set_mclk(&mmc2, 90000000U));
	TEST_EQ(3U, (test_mmio_read32(mmc2.sdhci_clk.reg_base) >> 24) & 0x3U);
	TEST_EQ(90000000U, sunxi_sdhci_get_mclk(&mmc2));
	TEST_EQ(50000000U, sunxi_sdhci_get_mclk(&mmc0));
	mmc0.sdhci_clk.default_clk_sel = 3U;
	TEST_EQ(-1, sunxi_sdhci_set_mclk(&mmc0, 50000000U));
	test_mmio_write32(mmc0.sdhci_clk.reg_base, 3U << 24);
	TEST_EQ(0U, sunxi_sdhci_get_mclk(&mmc0));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sdhci_dt_read_alias(&mmc2, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sdhci_dt_read_alias(&mmc2, "mmc-invalid-clock"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sdhci_dt_read_alias(&mmc2, "mmc-missing-clock"));
}
