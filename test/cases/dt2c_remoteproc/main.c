/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <string.h>

#include "syter_test.h"

const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {0};

void test_case_main(const char *case_dir) {
	sunxi_remoteproc_t primary = {0};
	sunxi_remoteproc_t rejected = {.entry = 0xdeadbeefU};
	sunxi_remoteproc_t secondary = {0};

	(void) case_dir;
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

	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected,
					"remoteproc-disabled", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected,
					"remoteproc-invalid-map", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected, "missing", NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(&rejected, NULL, NULL));
	TEST_EQ(DRIVER_ERROR_INVALID,
		 sunxi_remoteproc_dt_read_alias(NULL, "hifi4-primary", NULL));
	TEST_EQ(0xdeadbeefU, rejected.entry);
}
