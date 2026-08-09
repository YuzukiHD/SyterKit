/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <string.h>

#include "syter_test.h"

static const sunxi_remoteproc_ops_t test_ops = {0};

int sunxi_remoteproc_bind(sunxi_remoteproc_t *remoteproc,
			  sunxi_remoteproc_variant_t variant) {
	if (remoteproc == NULL ||
	    variant == SUNXI_REMOTEPROC_VARIANT_INVALID)
		return DRIVER_ERROR_INVALID;
	remoteproc->ops = &test_ops;
	return DRIVER_OK;
}

void test_case_main(const char *case_dir) {
	sunxi_remoteproc_t a27l2 = {0};
	sunxi_remoteproc_t ar100 = {0};
	sunxi_remoteproc_t c906 = {0};
	sunxi_remoteproc_t primary = {0};
	sunxi_remoteproc_t rejected = {.entry = 0xdeadbeefU};
	sunxi_remoteproc_t secondary = {0};
	sunxi_rtc_t rtc = {0};

	(void) case_dir;
	rtc.dt_node = syterkit_dt_alias_node("rtc0", SUNXI_RTC_COMPATIBLE);
	TEST_ASSERT(rtc.dt_node >= 0);
	TEST_EQ(DRIVER_OK,
		 sunxi_remoteproc_dt_read_alias(&primary, "hifi4-primary", NULL));
	TEST_EQ(DRIVER_OK,
		 sunxi_remoteproc_dt_read_alias(&secondary, "hifi4-secondary", NULL));
	TEST_ASSERT(primary.dt_node != secondary.dt_node);
	TEST_ASSERT(primary.ops == secondary.ops);
	TEST_EQ(3U, primary.register_count);
	TEST_EQ(3U, secondary.register_count);
	TEST_EQ(0x03000000U, primary.registers[0].base);
	TEST_EQ(0x01700000U, primary.registers[1].base);
	TEST_EQ(0x02001000U, primary.registers[2].base);
	TEST_EQ(0x03100000U, secondary.registers[0].base);
	TEST_EQ(0x01710000U, secondary.registers[1].base);
	TEST_EQ(0x02011000U, secondary.registers[2].base);
	TEST_ASSERT(primary.entry_from_elf);
	TEST_EQ(SUNXI_REMOTEPROC_FIRMWARE_ELF32, primary.format);
	TEST_EQ(1U, primary.firmware_count);
	TEST_ASSERT(strcmp("dsp0.elf", primary.firmware[0].name) == 0);
	TEST_EQ(0x10000000U, primary.firmware[0].load_address);
	TEST_EQ(0x00100000U, primary.firmware[0].region_size);
	TEST_EQ(2U, primary.address_map_count);
	TEST_EQ(0x10000000U, primary.address_map[0].device_start);
	TEST_EQ(0x1fffffffU, primary.address_map[0].device_end);
	TEST_EQ(0x30000000U, primary.address_map[0].physical_start);
	TEST_ASSERT(!secondary.entry_from_elf);
	TEST_EQ(0x100000U, secondary.entry);
	TEST_EQ(0U, secondary.address_map_count);

	TEST_EQ(DRIVER_OK,
		 sunxi_remoteproc_dt_read_alias(&c906, "c906", NULL));
	TEST_EQ(SUNXI_REMOTEPROC_FIRMWARE_ELF64, c906.format);
	TEST_EQ(2U, c906.register_count);
	TEST_EQ(0x02001000U, c906.registers[0].base);
	TEST_EQ(0x06010000U, c906.registers[1].base);
	TEST_EQ(3U, c906.firmware_count);
	TEST_ASSERT(strcmp("c906.elf", c906.firmware[0].name) == 0);
	TEST_ASSERT(strcmp("fw_jump.bin", c906.firmware[1].name) == 0);
	TEST_ASSERT(strcmp("u-boot.bin", c906.firmware[2].name) == 0);
	TEST_EQ(0x12000000U, c906.firmware[1].load_address);
	TEST_EQ(0x12100000U, c906.firmware[2].load_address);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&ar100, "ar100", NULL));
	TEST_EQ(DRIVER_OK,
		 sunxi_remoteproc_dt_read_alias(&ar100, "ar100", &rtc));
	TEST_EQ(SUNXI_REMOTEPROC_FIRMWARE_RAW, ar100.format);
	TEST_EQ(2U, ar100.register_count);
	TEST_EQ(0x03000000U, ar100.registers[0].base);
	TEST_EQ(0x07000400U, ar100.registers[1].base);
	TEST_ASSERT(!ar100.entry_from_elf);
	TEST_EQ(0U, ar100.entry);
	TEST_EQ(2U, ar100.address_map_count);
	TEST_EQ(0x00104000U, ar100.address_map[1].physical_start);

	TEST_EQ(DRIVER_OK,
		 sunxi_remoteproc_dt_read_alias(&a27l2, "a27l2", NULL));
	TEST_EQ(4U, a27l2.register_count);
	TEST_EQ(0x4a011000U, a27l2.registers[0].base);
	TEST_EQ(0x4a010000U, a27l2.registers[1].base);
	TEST_EQ(0x42001000U, a27l2.registers[2].base);
	TEST_EQ(0x49100000U, a27l2.registers[3].base);
	TEST_ASSERT(a27l2.entry_from_elf);

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected,
					"remoteproc-disabled", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected,
					"remoteproc-invalid-map", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected,
					"remoteproc-wrong", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected, "missing", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected, NULL, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(NULL, "hifi4-primary", NULL));
	TEST_EQ(0xdeadbeefU, rejected.entry);
}
