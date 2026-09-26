/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * eFEX parameter area: parse the host table once, apply it to the driver
 * structures of the application, and collect results for the host.  Only
 * structure fields are touched, so this links on every chip regardless of
 * which drivers the application initialises.
 */

#include <efex.h>

#include <drivers/gpio/gpio.h>

struct efex_param_area efex_param __attribute__((section(".efex.param"), used)) = {
	.hdr = {
		.magic = EFEX_PARAM_MAGIC,
		.version = EFEX_PARAM_VERSION,
		.hdr_size = sizeof(struct efex_param_hdr),
		.count = 0,
		.capacity = EFEX_PARAM_CAPACITY,
	},
};

/* 0 = not checked yet, 1 = host table accepted, -1 = rejected and reset. */
static int efex_param_state;

#define EFEX_PARAM_BUS_FIELDS (EFEX_PARAM_UART_DLEN + 1U)

/* Bus fields collected from the table; pins need all of them at once. */
struct efex_param_bus {
	uint32_t set;
	uint32_t value[EFEX_PARAM_BUS_FIELDS];
};

static uint32_t efex_param_checksum(void)
{
	uint32_t sum = 0;
	uint16_t index;

	for (index = 0; index < efex_param.hdr.count; index++)
		sum += efex_param.ent[index].key + efex_param.ent[index].value;

	return sum;
}

static bool efex_param_header_ok(void)
{
	const struct efex_param_hdr *hdr = &efex_param.hdr;

	if (hdr->magic != EFEX_PARAM_MAGIC || hdr->version != EFEX_PARAM_VERSION)
		return false;
	if (hdr->hdr_size != sizeof(struct efex_param_hdr) || hdr->capacity != EFEX_PARAM_CAPACITY)
		return false;
	if (hdr->count > EFEX_PARAM_CAPACITY)
		return false;

	return hdr->checksum == 0 || hdr->checksum == efex_param_checksum();
}

static bool efex_param_check(void)
{
	if (efex_param_state == 0) {
		efex_param_state = efex_param_header_ok() ? 1 : -1;
		if (efex_param_state < 0) {
			/* Keep an empty, well-formed table so results can still be reported. */
			efex_param.hdr.magic = EFEX_PARAM_MAGIC;
			efex_param.hdr.version = EFEX_PARAM_VERSION;
			efex_param.hdr.hdr_size = sizeof(struct efex_param_hdr);
			efex_param.hdr.capacity = EFEX_PARAM_CAPACITY;
			efex_param.hdr.count = 0;
		}
	}

	return efex_param_state > 0;
}

static bool efex_param_bus_get(const struct efex_param_bus *bus, uint32_t field, uint32_t *value)
{
	if ((bus->set & (1U << field)) == 0)
		return false;
	*value = bus->value[field];
	return true;
}

static void efex_param_apply_clk(const struct efex_param_bus *bus, sunxi_clk_t *clk)
{
	uint32_t value;

	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_GATE_REG, &value))
		clk->gate_reg_base = value;
	efex_param_bus_get(bus, EFEX_PARAM_BUS_GATE_BIT, &clk->gate_reg_offset);
	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_RST_REG, &value))
		clk->rst_reg_base = value;
	efex_param_bus_get(bus, EFEX_PARAM_BUS_RST_BIT, &clk->rst_reg_offset);
	efex_param_bus_get(bus, EFEX_PARAM_BUS_PARENT_CLK, &clk->parent_clk);
}

/*
 * Both pins sit on one pin controller.  Its first port is taken from the host
 * or derived from the default pin so that bank = port - first port still holds
 * after only the pin number is overridden.
 */
static void efex_param_apply_pins(const struct efex_param_bus *bus, gpio_mux_t *pin0, gpio_mux_t *pin1)
{
	gpio_mux_t *pins[2] = { pin0, pin1 };
	uint32_t bank0 = (pin0->pin >> PIO_NUM_IO_BITS) - pin0->bank;
	uint32_t value;
	int index;

	efex_param_bus_get(bus, EFEX_PARAM_BUS_GPIO_BANK0, &bank0);
	for (index = 0; index < 2; index++) {
		gpio_mux_t *pin = pins[index];

		if (efex_param_bus_get(bus, EFEX_PARAM_BUS_GPIO_BASE, &value))
			pin->base = value;
		efex_param_bus_get(bus, index ? EFEX_PARAM_BUS_PIN1 : EFEX_PARAM_BUS_PIN0, &pin->pin);
		pin->bank = (uint8_t) ((pin->pin >> PIO_NUM_IO_BITS) - bank0);
		if (efex_param_bus_get(bus, index ? EFEX_PARAM_BUS_MUX1 : EFEX_PARAM_BUS_MUX0, &value))
			pin->mux = (uint8_t) value;
	}
}

static void efex_param_apply_serial(const struct efex_param_bus *bus, sunxi_serial_t *uart)
{
	uint32_t value;

	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_BASE, &value))
		uart->base = value;
	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_ID, &value))
		uart->id = (uint8_t) value;
	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_RATE, &value))
		uart->baud_rate = (sunxi_serial_baudrate_t) value;
	if (efex_param_bus_get(bus, EFEX_PARAM_UART_PARITY, &value))
		uart->parity = (sunxi_serial_parity_t) value;
	if (efex_param_bus_get(bus, EFEX_PARAM_UART_STOP, &value))
		uart->stop = (sunxi_serial_stop_bit_t) value;
	if (efex_param_bus_get(bus, EFEX_PARAM_UART_DLEN, &value))
		uart->dlen = (sunxi_serial_dlen_t) value;
	efex_param_apply_clk(bus, &uart->uart_clk);
	efex_param_apply_pins(bus, &uart->gpio_pin.gpio_tx, &uart->gpio_pin.gpio_rx);
}

static void efex_param_apply_i2c(const struct efex_param_bus *bus, sunxi_i2c_t *i2c)
{
	uint32_t value;

	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_BASE, &value))
		i2c->base = value;
	if (efex_param_bus_get(bus, EFEX_PARAM_BUS_ID, &value))
		i2c->id = (uint8_t) value;
	efex_param_bus_get(bus, EFEX_PARAM_BUS_RATE, &i2c->speed);
	efex_param_apply_clk(bus, &i2c->i2c_clk);
	efex_param_apply_pins(bus, &i2c->gpio.gpio_scl, &i2c->gpio.gpio_sda);
}

/* Memory entries apply directly; returns false for a field the target lacks. */
static bool efex_param_apply_mem(uint32_t id, uint32_t index, uint32_t value, uint32_t *parameters,
				 size_t max_words, size_t *count, uintptr_t *memory_base, size_t *memory_size)
{
	switch (id) {
	case EFEX_PARAM_MEM_PARA:
		if (index >= max_words)
			return false;
		parameters[index] = value;
		return true;
	case EFEX_PARAM_MEM_PARA_COUNT:
		if (index != 0 || value > max_words)
			return false;
		*count = value;
		return true;
	case EFEX_PARAM_MEM_BASE:
		if (index != 0)
			return false;
		*memory_base = value;
		return true;
	case EFEX_PARAM_MEM_SIZE:
		if (index != 0)
			return false;
		*memory_size = value;
		return true;
	default:
		return false;
	}
}

/* Apply or collect one host entry; returns true when a target consumed it. */
static bool efex_param_apply_entry(const struct efex_param_targets *targets, uint32_t key, uint32_t value,
				   struct efex_param_bus *uart, struct efex_param_bus *twi)
{
	uint32_t group = (key >> 24) & 0x3fU;
	uint32_t id = (key >> 8) & 0xffffU;
	uint32_t index = key & 0xffU;

	switch (group) {
	case EFEX_PARAM_GROUP_UART:
	case EFEX_PARAM_GROUP_PMU_TWI: {
		bool is_uart = group == EFEX_PARAM_GROUP_UART;
		struct efex_param_bus *bus = is_uart ? uart : twi;
		uint32_t fields = is_uart ? EFEX_PARAM_BUS_FIELDS : EFEX_PARAM_BUS_PARENT_CLK + 1U;

		if ((is_uart ? targets->uart == NULL : targets->i2c == NULL) || id >= fields || index != 0)
			return false;
		bus->set |= 1U << id;
		bus->value[id] = value;
		return true;
	}
	case EFEX_PARAM_GROUP_PMU_RAIL:
		if (targets->rail_mv == NULL || id >= targets->pmu_count || index >= EFEX_PARAM_RAIL_MAX)
			return false;
		targets->rail_mv[id][index] = (int) value;
		return true;
	case EFEX_PARAM_GROUP_DRAM:
		return targets->dram != NULL &&
		       efex_param_apply_mem(id, index, value, targets->dram->parameters, SUNXI_DRAM_MAX_PARAM_WORDS,
					    &targets->dram->parameter_count, &targets->dram->memory_base,
					    &targets->dram->memory_size);
	case EFEX_PARAM_GROUP_PSRAM:
		return targets->psram != NULL &&
		       efex_param_apply_mem(id, index, value, targets->psram->parameters, SUNXI_PSRAM_MAX_PARAM_WORDS,
					    &targets->psram->parameter_count, &targets->psram->memory_base,
					    &targets->psram->memory_size);
	case EFEX_PARAM_GROUP_APP:
		if (targets->app == NULL || (key & 0xffffffU) >= targets->app_count)
			return false;
		targets->app[key & 0xffffffU] = value;
		return true;
	default:
		return false;
	}
}

bool efex_param_load(const struct efex_param_targets *targets)
{
	struct efex_param_bus uart = { 0 };
	struct efex_param_bus twi = { 0 };
	uint16_t index;

	if (!efex_param_check())
		return false;

	for (index = 0; index < efex_param.hdr.count; index++) {
		struct efex_param_ent *ent = &efex_param.ent[index];

		/* Entries the target wrote itself are results, not host overrides. */
		if ((ent->key & EFEX_PARAM_OUTPUT) != 0)
			continue;
		if (efex_param_apply_entry(targets, ent->key & ~EFEX_PARAM_FLAGS, ent->value, &uart, &twi))
			ent->key |= EFEX_PARAM_APPLIED;
	}

	if (targets->uart != NULL)
		efex_param_apply_serial(&uart, targets->uart);
	if (targets->i2c != NULL)
		efex_param_apply_i2c(&twi, targets->i2c);

	return true;
}

static struct efex_param_ent *efex_param_find(uint32_t key)
{
	uint16_t index;

	efex_param_check();
	key &= ~EFEX_PARAM_FLAGS;
	for (index = 0; index < efex_param.hdr.count; index++) {
		if ((efex_param.ent[index].key & ~EFEX_PARAM_FLAGS) == key)
			return &efex_param.ent[index];
	}

	return NULL;
}

int efex_param_put(uint32_t key, uint32_t value)
{
	struct efex_param_ent *ent = efex_param_find(key);

	if (ent == NULL) {
		if (efex_param.hdr.count >= EFEX_PARAM_CAPACITY)
			return -1;
		ent = &efex_param.ent[efex_param.hdr.count++];
		ent->key = key & ~EFEX_PARAM_FLAGS;
	}

	ent->key |= EFEX_PARAM_OUTPUT;
	ent->value = value;

	return 0;
}

int efex_param_put_array(uint32_t group, uint32_t id, const uint32_t *values, size_t count)
{
	size_t index;

	for (index = 0; index < count && index <= 0xffU; index++) {
		if (efex_param_put(EFEX_PARAM_KEY(group, id, index), values[index]) != 0)
			return -1;
	}

	return 0;
}

static void efex_param_report_mem(uint32_t group, const uint32_t *parameters, size_t count, uint32_t size_mb)
{
	efex_param_put_array(group, EFEX_PARAM_MEM_PARA, parameters, count);
	efex_param_put(EFEX_PARAM_KEY(group, EFEX_PARAM_MEM_SIZE_MB, 0), size_mb);
	efex_param_put(EFEX_PARAM_KEY(group, EFEX_PARAM_MEM_INIT_OK, 0), size_mb != 0U);
}

void efex_param_report_dram(const sunxi_dram_t *dram, uint32_t size_mb)
{
	efex_param_report_mem(EFEX_PARAM_GROUP_DRAM, dram->parameters, dram->parameter_count, size_mb);
}

void efex_param_report_psram(const sunxi_psram_t *psram, uint32_t size_mb)
{
	efex_param_report_mem(EFEX_PARAM_GROUP_PSRAM, psram->parameters, psram->parameter_count, size_mb);
}

void efex_param_finish(int ret)
{
	if (!efex_param_check())
		efex_param.hdr.status = EFEX_PARAM_STATUS_BAD;
	else
		efex_param.hdr.status = ret == 0 ? EFEX_PARAM_STATUS_DONE : EFEX_PARAM_STATUS_FAIL;
	efex_param.hdr.ret = ret;
	efex_param.hdr.checksum = efex_param_checksum();
}
