/* SPDX-License-Identifier: GPL-2.0+ */

#include <string.h>

/* Included so each scenario can reset the file-local validation state. */
#include "../../../core/efex_param.c"

#include "syter_test.h"

static const struct {
	const char *name;
	uint32_t key;
} keys[] = {
	{ "uart.base", EFEX_PARAM_UART(EFEX_PARAM_BUS_BASE) },
	{ "uart.rate", EFEX_PARAM_UART(EFEX_PARAM_BUS_RATE) },
	{ "uart.pin0", EFEX_PARAM_UART(EFEX_PARAM_BUS_PIN0) },
	{ "uart.mux1", EFEX_PARAM_UART(EFEX_PARAM_BUS_MUX1) },
	{ "uart.gpio_bank0", EFEX_PARAM_UART(EFEX_PARAM_BUS_GPIO_BANK0) },
	{ "uart.parent_clk", EFEX_PARAM_UART(EFEX_PARAM_BUS_PARENT_CLK) },
	{ "uart.dlen", EFEX_PARAM_UART(EFEX_PARAM_UART_DLEN) },
	{ "twi.rate", EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_RATE) },
	{ "twi.pin1", EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_PIN1) },
	{ "twi.rst_bit", EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_RST_BIT) },
	{ "rail.1.2", EFEX_PARAM_PMU_RAIL(1, 2) },
	{ "dram.para[31]", EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA, 31) },
	{ "dram.para_count", EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA_COUNT, 0) },
	{ "dram.size", EFEX_PARAM_DRAM(EFEX_PARAM_MEM_SIZE, 0) },
	{ "dram.size_mb", EFEX_PARAM_DRAM(EFEX_PARAM_MEM_SIZE_MB, 0) },
	{ "psram.init_ok", EFEX_PARAM_PSRAM(EFEX_PARAM_MEM_INIT_OK, 0) },
	{ "app.1795", EFEX_PARAM_APP(1795) },
};

/* Reset to the linked default image contents plus the given host entries. */
static void load(const struct efex_param_ent *entries, uint16_t count, bool with_checksum)
{
	memset(&efex_param, 0, sizeof(efex_param));
	efex_param.hdr.magic = EFEX_PARAM_MAGIC;
	efex_param.hdr.version = EFEX_PARAM_VERSION;
	efex_param.hdr.hdr_size = sizeof(struct efex_param_hdr);
	efex_param.hdr.capacity = EFEX_PARAM_CAPACITY;
	efex_param.hdr.count = count;
	if (count)
		memcpy(efex_param.ent, entries, count * sizeof(*entries));
	if (with_checksum)
		efex_param.hdr.checksum = efex_param_checksum();
	efex_param_state = 0;
}

static uint32_t parse_hex(const char *text)
{
	uint32_t value = 0;

	if (text[0] == '0' && text[1] == 'x')
		text += 2;
	for (; *text; text++)
		value = value << 4 | (uint32_t)(*text <= '9' ? *text - '0' : (*text | 0x20) - 'a' + 10);

	return value;
}

static void test_keys_match_table(const char *case_dir)
{
	char data[TEST_DATA_MAX];
	char *line = data;
	size_t index = 0;

	TEST_ASSERT(test_load_data(case_dir, "data/keys.txt", data, sizeof(data)) > 0);
	while (*line) {
		char *end = strchr(line, '\n');
		char *value = strchr(line, ' ');

		if (end != NULL)
			*end = '\0';
		TEST_ASSERT(value != NULL && index < sizeof(keys) / sizeof(keys[0]));
		if (value == NULL || index >= sizeof(keys) / sizeof(keys[0]))
			return;
		*value++ = '\0';
		TEST_STREQ(keys[index].name, line);
		TEST_EQ(keys[index].key, parse_hex(value));
		index++;
		if (end == NULL)
			break;
		line = end + 1;
	}
	TEST_EQ(sizeof(keys) / sizeof(keys[0]), index);
}

static const struct efex_param_targets no_targets;

static void test_empty_table(void)
{
	sunxi_serial_t uart = { .baud_rate = UART_BAUDRATE_115200 };

	load(NULL, 0, false);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .uart = &uart }));
	TEST_EQ(UART_BAUDRATE_115200, uart.baud_rate);
	efex_param_finish(0);
	TEST_EQ(EFEX_PARAM_STATUS_DONE, efex_param.hdr.status);
	TEST_EQ(0, efex_param.hdr.count);
}

static void test_applied_flags(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_RATE), UART_BAUDRATE_1500000 },
		{ EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_RATE), SUNXI_I2C_SPEED_100K },
		{ EFEX_PARAM_OUTPUT | EFEX_PARAM_UART(EFEX_PARAM_BUS_ID), 3 },
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_RATE) + 0x40, 1 }, /* unknown field */
	};
	sunxi_serial_t uart = { .id = 1, .baud_rate = UART_BAUDRATE_115200 };

	load(host, 4, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .uart = &uart }));
	TEST_EQ(UART_BAUDRATE_1500000, uart.baud_rate);
	TEST_EQ(EFEX_PARAM_APPLIED | host[0].key, efex_param.ent[0].key);
	/* No I2C target: the entry stays unmarked. */
	TEST_EQ(host[1].key, efex_param.ent[1].key);
	/* Results are not read back as host overrides. */
	TEST_EQ(1, uart.id);
	TEST_EQ(host[2].key, efex_param.ent[2].key);
	TEST_EQ(host[3].key, efex_param.ent[3].key);
}

static void test_put_updates_and_appends(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA, 0), 0x4b0 },
	};
	const uint32_t trained[2] = { 0x4b1, 0x8 };
	static sunxi_dram_t dram;

	load(host, 1, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .dram = &dram }));
	TEST_EQ(0x4b0, dram.parameters[0]);
	TEST_EQ(0, efex_param_put_array(EFEX_PARAM_GROUP_DRAM, EFEX_PARAM_MEM_PARA, trained, 2));
	TEST_EQ(2, efex_param.hdr.count);
	TEST_EQ(EFEX_PARAM_APPLIED | EFEX_PARAM_OUTPUT | host[0].key, efex_param.ent[0].key);
	TEST_EQ(0x4b1, efex_param.ent[0].value);
	TEST_EQ(EFEX_PARAM_OUTPUT | EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA, 1), efex_param.ent[1].key);

	efex_param_finish(-1);
	TEST_EQ(EFEX_PARAM_STATUS_FAIL, efex_param.hdr.status);
	TEST_EQ((uint32_t)-1, (uint32_t)efex_param.hdr.ret);
	TEST_EQ(efex_param_checksum(), efex_param.hdr.checksum);
}

static void test_full_table(void)
{
	uint32_t index;

	load(NULL, 0, false);
	for (index = 0; index < EFEX_PARAM_CAPACITY; index++)
		TEST_EQ(0, efex_param_put(EFEX_PARAM_APP(index), index));
	TEST_EQ(-1, efex_param_put(EFEX_PARAM_APP(0xffffff), 1));
	/* Updating an existing key still works when full. */
	TEST_EQ(0, efex_param_put(EFEX_PARAM_APP(5), 50));
	TEST_EQ(50, efex_param.ent[5].value);
}

static void test_rejected_tables(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_RATE), UART_BAUDRATE_1500000 },
	};
	sunxi_serial_t uart = { .baud_rate = UART_BAUDRATE_115200 };
	const struct efex_param_targets targets = { .uart = &uart };

	/* Wrong checksum: defaults kept, results still reported. */
	load(host, 1, true);
	efex_param.hdr.checksum ^= 1;
	TEST_ASSERT(!efex_param_load(&targets));
	TEST_EQ(UART_BAUDRATE_115200, uart.baud_rate);
	TEST_EQ(0, efex_param.hdr.count);
	TEST_EQ(0, efex_param_put(EFEX_PARAM_DRAM(EFEX_PARAM_MEM_SIZE_MB, 0), 512));
	efex_param_finish(0);
	TEST_EQ(EFEX_PARAM_STATUS_BAD, efex_param.hdr.status);
	TEST_EQ(1, efex_param.hdr.count);

	/* Count beyond capacity. */
	load(host, 1, false);
	efex_param.hdr.count = EFEX_PARAM_CAPACITY + 1;
	TEST_ASSERT(!efex_param_load(&no_targets));

	/* Garbage header, e.g. SRAM never written by an older host. */
	load(host, 1, false);
	efex_param.hdr.magic = 0xffffffffU;
	TEST_ASSERT(!efex_param_load(&no_targets));
	TEST_EQ(EFEX_PARAM_MAGIC, efex_param.hdr.magic);
	TEST_EQ(EFEX_PARAM_CAPACITY, efex_param.hdr.capacity);
}

static void test_apply_serial(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_RATE), UART_BAUDRATE_1500000 },
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_PIN0), GPIO_PIN(GPIO_PORTH, 9) },
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_PIN1), GPIO_PIN(GPIO_PORTH, 10) },
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_MUX0), 5 },
		{ EFEX_PARAM_UART(EFEX_PARAM_BUS_MUX1), 5 },
	};
	sunxi_serial_t uart = {
		.base = 0x2500000,
		.uart_clk = { .gate_reg_offset = 0, .parent_clk = 24000000 },
		.gpio_pin = {
			.gpio_tx = { .base = 0x2000000, .pin = GPIO_PIN(GPIO_PORTB, 9), .bank = GPIO_PORTB, .mux = 2 },
			.gpio_rx = { .base = 0x2000000, .pin = GPIO_PIN(GPIO_PORTB, 10), .bank = GPIO_PORTB, .mux = 2 },
		},
		.baud_rate = UART_BAUDRATE_115200,
	};

	/* Pin before mux in the table must not matter; the bus is applied as a whole. */
	load(host, 5, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .uart = &uart }));
	TEST_EQ(UART_BAUDRATE_1500000, uart.baud_rate);
	TEST_EQ(0x2500000, uart.base);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 9), uart.gpio_pin.gpio_tx.pin);
	TEST_EQ(GPIO_PORTH, uart.gpio_pin.gpio_tx.bank);
	TEST_EQ(5, uart.gpio_pin.gpio_rx.mux);
	TEST_EQ(0x2000000, uart.gpio_pin.gpio_rx.base);
	TEST_EQ(24000000, uart.uart_clk.parent_clk);
}

static void test_apply_i2c_rpio(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_RATE), SUNXI_I2C_SPEED_100K },
		{ EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_PIN0), GPIO_PIN(GPIO_PORTM, 0) },
		{ EFEX_PARAM_PMU_TWI(EFEX_PARAM_BUS_PIN1), GPIO_PIN(GPIO_PORTM, 1) },
		{ EFEX_PARAM_PMU_TWI(EFEX_PARAM_UART_DLEN), 8 }, /* UART-only field */
	};
	/* R_PIO: port L is bank 0 of the controller. */
	sunxi_i2c_t i2c = {
		.speed = SUNXI_I2C_SPEED_400K,
		.gpio = {
			.gpio_scl = { .base = 0x7022000, .pin = GPIO_PIN(GPIO_PORTL, 0), .bank = 0, .mux = 2 },
			.gpio_sda = { .base = 0x7022000, .pin = GPIO_PIN(GPIO_PORTL, 1), .bank = 0, .mux = 2 },
		},
	};

	load(host, 4, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .i2c = &i2c }));
	TEST_EQ(SUNXI_I2C_SPEED_100K, i2c.speed);
	TEST_EQ(1, i2c.gpio.gpio_scl.bank);
	TEST_EQ(1, i2c.gpio.gpio_sda.bank);
	TEST_EQ(2, i2c.gpio.gpio_sda.mux);
	TEST_EQ(host[3].key, efex_param.ent[3].key);
}

static void test_apply_rails_and_app(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_PMU_RAIL(1, 1), 940 }, { EFEX_PARAM_PMU_RAIL(2, 0), 1200 }, /* no third PMU */
		{ EFEX_PARAM_APP(1), 0xabcd }, { EFEX_PARAM_APP(2), 0x1234 }, /* beyond app_count */
	};
	int rail_mv[][EFEX_PARAM_RAIL_MAX] = { { 1100, 920 }, { 1000, 1000 } };
	uint32_t app[2] = { 7, 8 };

	load(host, 4, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){
		.rail_mv = rail_mv,
		.pmu_count = 2,
		.app = app,
		.app_count = 2,
	}));
	TEST_EQ(1100, rail_mv[0][0]);
	TEST_EQ(940, rail_mv[1][1]);
	TEST_EQ(0xabcd, app[1]);
	TEST_EQ(7, app[0]);
	TEST_EQ(EFEX_PARAM_APPLIED | host[0].key, efex_param.ent[0].key);
	TEST_EQ(host[1].key, efex_param.ent[1].key);
	TEST_EQ(host[3].key, efex_param.ent[3].key);
}

static void test_apply_and_report_dram(void)
{
	const struct efex_param_ent host[] = {
		{ EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA, 0), 792 },
		{ EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA_COUNT, 0), 4 },
		{ EFEX_PARAM_DRAM(EFEX_PARAM_MEM_SIZE, 0), 0x40000000 },
	};
	static sunxi_dram_t dram = {
		.parameters = { 1200, 3, 7, 9 },
		.parameter_count = 32,
		.memory_size = 0x80000000U,
	};

	load(host, 3, true);
	TEST_ASSERT(efex_param_load(&(const struct efex_param_targets){ .dram = &dram }));
	TEST_EQ(792, dram.parameters[0]);
	TEST_EQ(3, dram.parameters[1]);
	TEST_EQ(4, dram.parameter_count);
	TEST_EQ(0x40000000, dram.memory_size);

	dram.parameters[1] = 0x33;
	efex_param_report_dram(&dram, 1024);
	/* 3 host entries, PARA[1..3] appended (PARA[0] updated in place), SIZE_MB, INIT_OK. */
	TEST_EQ(8, efex_param.hdr.count);
	TEST_EQ(EFEX_PARAM_OUTPUT | EFEX_PARAM_APPLIED | host[0].key, efex_param.ent[0].key);
	TEST_EQ(EFEX_PARAM_OUTPUT | EFEX_PARAM_DRAM(EFEX_PARAM_MEM_PARA, 1), efex_param.ent[3].key);
	TEST_EQ(0x33, efex_param.ent[3].value);
	TEST_EQ(EFEX_PARAM_OUTPUT | EFEX_PARAM_DRAM(EFEX_PARAM_MEM_INIT_OK, 0), efex_param.ent[7].key);
	TEST_EQ(1, efex_param.ent[7].value);
	TEST_EQ(1024, efex_param.ent[6].value);
}

void test_case_main(const char *case_dir)
{
	test_keys_match_table(case_dir);
	test_empty_table();
	test_applied_flags();
	test_put_updates_and_appends();
	test_full_table();
	test_rejected_tables();
	test_apply_serial();
	test_apply_i2c_rpio();
	test_apply_rails_and_app();
	test_apply_and_report_dram();
}
