/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/sid-dt.h>

#include <sid-internal.h>

#include "syter_test.h"

static uintptr_t last_mmio_read;
static uint32_t mmio_read_value;
static uint32_t current_efuse_offset;
static uint32_t efuse_read_offsets[128];
static size_t efuse_read_count;
static uint32_t efuse_program_value;
static uint32_t hv_switch_values[2];
static size_t hv_switch_write_count;

uint32_t test_mmio_read32(uintptr_t address)
{
	last_mmio_read = address;
	if (address == 0x1060U) {
		if (efuse_read_count < sizeof(efuse_read_offsets) / sizeof(efuse_read_offsets[0]))
			efuse_read_offsets[efuse_read_count] = current_efuse_offset;
		efuse_read_count++;
	}
	return mmio_read_value;
}

void test_mmio_write32(uintptr_t address, uint32_t value)
{
	if (address == 0x1040U && (value & 0x3U) == 0x2U)
		current_efuse_offset = (value >> 16U) & 0x1ffU;
	else if (address == 0x1050U)
		efuse_program_value = value;
	else if (address == 0x3000U) {
		if (hv_switch_write_count < sizeof(hv_switch_values) / sizeof(hv_switch_values[0]))
			hv_switch_values[hv_switch_write_count] = value;
		hv_switch_write_count++;
	}
}

void printk(int level, const char *format, ...)
{
	(void)level;
	(void)format;
}

void test_case_main(const char *case_dir)
{
	size_t expected_read_count;
	sunxi_sid_t sid0 = { 0 };
	sunxi_sid_t sid1 = { 0 };
	sunxi_sid_t rejected = { .base = 0xdeadbeefU };
	sunxi_sid_t wrapped = {
		.base = (uintptr_t)-0x100,
		.size = 0x400U,
	};

	(void)case_dir;
	TEST_EQ(DRIVER_OK, sunxi_sid_dt_read_alias(&sid0, "sid0"));
	TEST_EQ(0x1000U, sid0.base);
	TEST_EQ(0x400U, sid0.size);
	TEST_EQ(0x3000U, sid0.efuse_hv_switch);

	TEST_EQ(DRIVER_OK, sunxi_sid_dt_read_alias(&sid1, "sid1"));
	TEST_EQ(0x2000U, sid1.base);
	TEST_EQ(0x400U, sid1.size);
	TEST_EQ(0U, sid1.efuse_hv_switch);
	TEST_ASSERT(sid0.dt_node != sid1.dt_node);

	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, "sid-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, "sid-short"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, "sid-bad-switch"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, "sid-wrong-compatible"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(&rejected, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_alias(NULL, "sid0"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_sid_dt_read_config(&rejected, -1));
	TEST_EQ(0xdeadbeefU, rejected.base);

	mmio_read_value = 0x12345678U;
	last_mmio_read = 0U;
	TEST_EQ(0x12345678U, sunxi_sid_read_sram(&sid0, 0x3cU));
	TEST_EQ(0x123cU, last_mmio_read);
	last_mmio_read = 0U;
	TEST_EQ(0U, sunxi_sid_read_sram(&sid0, 1U));
	TEST_EQ(0U, last_mmio_read);
	TEST_EQ(0U, sunxi_sid_read_sram(&sid0, 0x200U));
	TEST_EQ(0U, last_mmio_read);
	TEST_EQ(0U, sunxi_sid_read_sram(&wrapped, 0U));
	TEST_EQ(0U, last_mmio_read);
	TEST_EQ(0U, sunxi_sid_read_sram(NULL, 0U));

	hv_switch_write_count = 0U;
	sunxi_efuse_write(&sid0, 0x88U, 0xcafebabeU);
	TEST_EQ(2U, hv_switch_write_count);
	TEST_EQ(1U, hv_switch_values[0]);
	TEST_EQ(0U, hv_switch_values[1]);
	TEST_EQ(0xcafebabeU, efuse_program_value);
	sunxi_efuse_write(&sid1, 0x88U, 0xdeadbeefU);
	TEST_EQ(2U, hv_switch_write_count);
	TEST_EQ(0xcafebabeU, efuse_program_value);

	efuse_read_count = 0U;
	sunxi_efuse_dump(&sid0);
	TEST_EQ(11U, sunxi_sid_section_count);
	TEST_EQ(0x44U, sunxi_sid_sections[8].offset);
	TEST_EQ(800U, sunxi_sid_sections[8].size_bits);
	TEST_EQ(0xc8U, sunxi_sid_sections[10].offset);
	TEST_EQ(448U, sunxi_sid_sections[10].size_bits);

	expected_read_count = 0U;
	for (size_t section = 0; section < sunxi_sid_section_count; section++)
		expected_read_count += (sunxi_sid_sections[section].size_bits + 31U) / 32U;
	TEST_EQ(64U, expected_read_count);
	TEST_EQ(expected_read_count, efuse_read_count);
	TEST_ASSERT(expected_read_count <= sizeof(efuse_read_offsets) / sizeof(efuse_read_offsets[0]));
	if (expected_read_count > sizeof(efuse_read_offsets) / sizeof(efuse_read_offsets[0]))
		return;

	expected_read_count = 0U;
	for (size_t section = 0; section < sunxi_sid_section_count; section++) {
		const sunxi_sid_section_t *entry = &sunxi_sid_sections[section];
		size_t word_count = (entry->size_bits + 31U) / 32U;

		for (size_t index = 0; index < word_count; index++)
			TEST_EQ(entry->offset + index * sizeof(uint32_t), efuse_read_offsets[expected_read_count++]);
	}
}
