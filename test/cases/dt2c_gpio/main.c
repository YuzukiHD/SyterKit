/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/gpio-dt.h>
#include <dt-compatible/pinctrl-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	const dt2c_fdt32_t *pins;
	gpio_mux_t direct = {0};
	gpio_mux_t r_tx = {0};
	gpio_mux_t rx = {0};
	gpio_mux_t tx = {0};
	sunxi_gpio_t controller = {0};
	sunxi_gpio_t group_controller = {0};
	sunxi_gpio_t r_controller = {0};
	int bad_count_consumer;
	int bad_parent_consumer;
	int consumer;
	int r_consumer;

	(void) case_dir;
	TEST_EQ(DRIVER_OK, sunxi_gpio_dt_read_alias(&controller, "gpio0"));
	TEST_EQ(0x02000000U, controller.base);
	TEST_EQ(GPIO_PORTA, controller.bank_base);
	TEST_EQ(11, controller.bank_count);
	TEST_ASSERT(controller.dt_node >= 0);
	TEST_EQ(DRIVER_OK, sunxi_gpio_dt_read_alias(&r_controller, "gpio1"));
	TEST_EQ(0x07022000U, r_controller.base);
	TEST_EQ(GPIO_PORTL, r_controller.bank_base);
	TEST_EQ(3, r_controller.bank_count);
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, "gpio-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, "gpio-invalid-cells"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, "gpio-invalid-reg"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, "gpio-invalid-bank"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_alias(&controller, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_gpio_dt_read_config(NULL, controller.dt_node));

	consumer = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE, "/consumer");
	TEST_ASSERT(consumer >= 0);
	pins = syterkit_dt_pinctrl_cells(consumer, 6, &group_controller);
	TEST_ASSERT(pins != NULL);
	TEST_EQ(0x02000000U, group_controller.base);
	TEST_EQ(GPIO_PORTA, group_controller.bank_base);
	TEST_ASSERT(syterkit_dt_pinctrl_gpio(
			pins, 0, &group_controller, &tx));
	TEST_ASSERT(syterkit_dt_pinctrl_gpio(
			pins, 3, &group_controller, &rx));
	TEST_EQ(GPIO_PIN(GPIO_PORTB, 8), tx.pin);
	TEST_EQ(GPIO_PERIPH_MUX6, tx.mux);
	TEST_EQ(0x02000000U, tx.base);
	TEST_EQ(1, tx.bank);
	TEST_EQ(GPIO_PIN(GPIO_PORTB, 9), rx.pin);
	TEST_EQ(GPIO_PERIPH_MUX6, rx.mux);

	TEST_ASSERT(sunxi_gpio_dt_read_property(
			&direct, consumer, "reset-gpio"));
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 3), direct.pin);
	TEST_EQ(GPIO_OUTPUT, direct.mux);
	TEST_EQ(0x07022000U, direct.base);
	TEST_EQ(0, direct.bank);
	TEST_ASSERT(!sunxi_gpio_dt_read_property(
			&direct, consumer, "wrong-controller-gpio"));
	TEST_ASSERT(!sunxi_gpio_dt_read_property(
			&direct, consumer, "invalid-pin-gpio"));
	TEST_ASSERT(!sunxi_gpio_dt_read_property(
			&direct, consumer, "disabled-gpio"));
	TEST_ASSERT(!sunxi_gpio_dt_read_property(
			&direct, consumer, "missing-gpio"));

	r_consumer = dt2c_fdt_path_offset(
			DT2C_FDT_COMPILED_TREE, "/r-consumer");
	TEST_ASSERT(r_consumer >= 0);
	pins = syterkit_dt_pinctrl_cells(r_consumer, 6, &group_controller);
	TEST_ASSERT(pins != NULL);
	TEST_EQ(0x07022000U, group_controller.base);
	TEST_EQ(GPIO_PORTL, group_controller.bank_base);
	TEST_ASSERT(syterkit_dt_pinctrl_gpio(
			pins, 0, &group_controller, &r_tx));
	TEST_EQ(GPIO_PIN(GPIO_PORTL, 3), r_tx.pin);
	TEST_EQ(0x07022000U, r_tx.base);
	TEST_EQ(0, r_tx.bank);

	bad_parent_consumer = dt2c_fdt_path_offset(
			DT2C_FDT_COMPILED_TREE, "/bad-parent-consumer");
	bad_count_consumer = dt2c_fdt_path_offset(
			DT2C_FDT_COMPILED_TREE, "/bad-count-consumer");
	TEST_ASSERT(syterkit_dt_pinctrl(
			bad_parent_consumer, NULL, &group_controller) == NULL);
	TEST_ASSERT(syterkit_dt_pinctrl(
			bad_count_consumer, NULL, &group_controller) == NULL);
}
