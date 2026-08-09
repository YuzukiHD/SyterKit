/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTER_TEST_IO_H__
#define __SYTER_TEST_IO_H__

#include <stdint.h>

#define BIT(index) (1UL << (index))

uint32_t test_mmio_read32(uintptr_t address);
void test_mmio_write32(uintptr_t address, uint32_t value);

static inline uint32_t readl(uintptr_t address) {
	return test_mmio_read32(address);
}

static inline void writel(uint32_t value, uintptr_t address) {
	test_mmio_write32(address, value);
}

static inline uint32_t read32(uintptr_t address) {
	return test_mmio_read32(address);
}

static inline void write32(uintptr_t address, uint32_t value) {
	test_mmio_write32(address, value);
}

#endif
