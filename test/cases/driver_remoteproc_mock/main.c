/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <lib/elf/elf_loader.h>

#include "syter_test.h"

static uintptr_t expected_entry;
static uintptr_t prepared_entry;
static unsigned int elf32_entry_calls;
static unsigned int elf32_load_calls;
static unsigned int elf64_entry_calls;

phys_addr_t elf32_get_entry_addr(phys_addr_t base) {
	(void) base;
	elf32_entry_calls++;
	return (phys_addr_t) expected_entry;
}

phys_addr_t elf64_get_entry_addr(phys_addr_t base) {
	(void) base;
	elf64_entry_calls++;
	return (phys_addr_t) expected_entry;
}

int load_elf32_image_remap(phys_addr_t image, vaddr_map_t *mapping) {
	(void) image;
	TEST_EQ(0U, mapping->range_size);
	elf32_load_calls++;
	return 0;
}

int load_elf32_image(phys_addr_t image) {
	(void) image;
	return 0;
}

int load_elf64_image(phys_addr_t image) {
	(void) image;
	return 0;
}

static int test_prepare(sunxi_remoteproc_t *remoteproc) {
	prepared_entry = remoteproc->entry;
	return DRIVER_OK;
}

static const sunxi_remoteproc_ops_t test_ops = {
	.prepare = test_prepare,
};

void test_case_main(const char *case_dir) {
	uint8_t firmware[64] = {0};
	sunxi_remoteproc_t explicit_entry = {
		.format = SUNXI_REMOTEPROC_FIRMWARE_ELF32,
		.firmware = {{
			.load_address = (uintptr_t) firmware,
			.region_size = sizeof(firmware),
		}},
		.firmware_count = 1U,
		.entry = 0x4000U,
		.entry_from_elf = false,
		.ops = &test_ops,
	};
	sunxi_remoteproc_t from_elf = explicit_entry;

	(void) case_dir;
	expected_entry = 0x12345678U;
	from_elf.entry = 0U;
	from_elf.entry_from_elf = true;

	TEST_EQ(DRIVER_OK, sunxi_remoteproc_prepare(&from_elf));
	TEST_EQ(expected_entry, from_elf.entry);
	TEST_EQ(expected_entry, prepared_entry);
	TEST_EQ(1U, elf32_entry_calls);
	TEST_EQ(0U, elf32_load_calls);

	TEST_EQ(DRIVER_OK, sunxi_remoteproc_load(&from_elf));
	TEST_EQ(2U, elf32_entry_calls);
	TEST_EQ(1U, elf32_load_calls);

	prepared_entry = 0U;
	TEST_EQ(DRIVER_OK, sunxi_remoteproc_prepare(&explicit_entry));
	TEST_EQ(0x4000U, prepared_entry);
	TEST_EQ(2U, elf32_entry_calls);
	TEST_EQ(0U, elf64_entry_calls);
}
