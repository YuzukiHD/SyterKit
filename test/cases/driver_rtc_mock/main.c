/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <drivers/rtc/rtc.h>

#include "syter_test.h"

static uint32_t registers[16];
static unsigned int barrier_count;

uint32_t test_mmio_read32(uintptr_t address)
{
	return registers[(address >> 2U) & 15U];
}

void test_mmio_write32(uintptr_t address, uint32_t value)
{
	registers[(address >> 2U) & 15U] = value;
}

void test_data_sync_barrier(void)
{
	barrier_count++;
}

uint32_t get_init_timestamp(void)
{
	return 0x12345678U;
}

static uint32_t parse_hex(const char *text)
{
	uint32_t value = 0;

	while (*text) {
		unsigned int digit;

		if (*text >= '0' && *text <= '9')
			digit = (unsigned int)(*text - '0');
		else if (*text >= 'a' && *text <= 'f')
			digit = (unsigned int)(*text - 'a' + 10);
		else if (*text >= 'A' && *text <= 'F')
			digit = (unsigned int)(*text - 'A' + 10);
		else {
			text++;
			continue;
		}
		value = (value << 4U) | digit;
		text++;
	}
	return value;
}

void test_case_main(const char *case_dir)
{
	char data[TEST_DATA_MAX];
	sunxi_rtc_t rtc = {
		.data_base = 0x100U,
		.data_size = sizeof(registers),
	};
	uint32_t boot_flag;
	int length;

	length = test_load_data(case_dir, "data/boot-flag.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;
	boot_flag = parse_hex(data);

	rtc_set_fel_flag(&rtc);
	TEST_EQ(1, rtc_probe_fel_flag(&rtc));
	rtc_clear_fel_flag(&rtc);
	TEST_EQ(0, rtc_probe_fel_flag(&rtc));
	TEST_EQ(0, rtc_set_bootmode_flag(&rtc, (uint8_t)boot_flag));
	TEST_EQ((uint8_t)boot_flag, rtc_get_bootmode_flag(&rtc));
	rtc_set_dram_para(&rtc, 0x87654321U);
	TEST_EQ(0x87654321U, rtc_read_data(&rtc, RTC_DRAM_PARA_ADDR));
	rtc_set_start_time_ms(&rtc);
	TEST_EQ(0x12345678U, rtc_read_data(&rtc, RTC_FEL_INDEX));
	TEST_ASSERT(barrier_count >= 5U);
}
