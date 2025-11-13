/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>
#include <types.h>

/**
 * Define a bit mask for the specified bit position.
 *
 * @param x The bit position to generate the bit mask for.
 */
#define BIT(x) (1 << x)

/**
 * @brief Read or write a 32-bit register at the specified address.
 *
 * This macro casts the given address `x` to a pointer to a volatile 
 * 32-bit integer and returns the value at that address. This is useful 
 * for directly accessing hardware registers.
 *
 * @param x The address of the target register.
 * @return The value of the target register.
 */
#define REG32(x) (*((volatile uint32_t *) (x)))

/**
 * Clear and set bits in a 32-bit address.
 *
 * @param addr The address to perform the operation on.
 * @param clear The bits to clear.
 * @param set The bits to set.
 */
#define clrsetbits_le32(addr, clear, set) write32((addr), (read32(addr) & ~(clear)) | (set))

/**
 * Set bits in a 32-bit address.
 *
 * @param addr The address to perform the operation on.
 * @param set The bits to set.
 */
#define setbits_le32(addr, set) write32((addr), read32(addr) | (set))

/**
 * Clear bits in a 32-bit address.
 *
 * @param addr The address to perform the operation on.
 * @param clear The bits to clear.
 */
#define clrbits_le32(addr, clear) write32((addr), read32(addr) & ~(clear))

/**
 * Read a byte from the specified address.
 *
 * @param addr The address to read from.
 */
#define readb(addr) read8(addr)

/**
 * Write a byte value to the specified address.
 *
 * @param val The value to write.
 * @param addr The address to write to.
 */
#define writeb(val, addr) write8((addr), (val))

/**
 * Read a 16-bit word from the specified address.
 *
 * @param addr The address to read from.
 */
#define readw(addr) read16(addr)

/**
 * Write a 16-bit word value to the specified address.
 *
 * @param val The value to write.
 * @param addr The address to write to.
 */
#define writew(val, addr) write16((addr), (val))

/**
 * Read a 32-bit double word from the specified address.
 *
 * @param addr The address to read from.
 */
#define readl(addr) read32(addr)

/**
 * Write a 32-bit double word value to the specified address.
 *
 * @param val The value to write.
 * @param addr The address to write to.
 */
#define writel(val, addr) write32((addr), (val))

/**
 * Inline function to read an 8-bit value from the specified address.
 *
 * @param addr The address to read from.
 * @return The 8-bit value read from the address.
 */
static inline __attribute__((__always_inline__)) uint8_t read8(virtual_addr_t addr) {
	return (*((volatile uint8_t *) (addr)));
}

/**
 * Inline function to read a 16-bit value from the specified address.
 *
 * @param addr The address to read from.
 * @return The 16-bit value read from the address.
 */
static inline __attribute__((__always_inline__)) uint16_t read16(virtual_addr_t addr) {
	return (*((volatile uint16_t *) (addr)));
}

/**
 * Inline function to read a 32-bit value from the specified address.
 *
 * @param addr The address to read from.
 * @return The 32-bit value read from the address.
 */
static inline __attribute__((__always_inline__)) uint32_t read32(virtual_addr_t addr) {
	return (*((volatile uint32_t *) (addr)));
}

/**
 * Inline function to read a 64-bit value from the specified address.
 *
 * @param addr The address to read from.
 * @return The 64-bit value read from the address.
 */
static inline __attribute__((__always_inline__)) uint64_t read64(virtual_addr_t addr) {
	return (*((volatile uint64_t *) (addr)));
}

/**
 * Inline function to write an 8-bit value to the specified address.
 *
 * @param addr The address to write to.
 * @param value The 8-bit value to write.
 */
static inline __attribute__((__always_inline__)) void write8(virtual_addr_t addr, uint8_t value) {
	*((volatile uint8_t *) (addr)) = value;
}

/**
 * Inline function to write a 16-bit value to the specified address.
 *
 * @param addr The address to write to.
 * @param value The 16-bit value to write.
 */
static inline __attribute__((__always_inline__)) void write16(virtual_addr_t addr, uint16_t value) {
	*((volatile uint16_t *) (addr)) = value;
}

/**
 * Inline function to write a 32-bit value to the specified address.
 *
 * @param addr The address to write to.
 * @param value The 32-bit value to write.
 */
static inline __attribute__((__always_inline__)) void write32(virtual_addr_t addr, uint32_t value) {
	*((volatile uint32_t *) (addr)) = value;
}

/**
 * Inline function to write a 64-bit value to the specified address.
 *
 * @param addr The address to write to.
 * @param value The 64-bit value to write.
 */
static inline __attribute__((__always_inline__)) void write64(virtual_addr_t addr, uint64_t value) {
	*((volatile uint64_t *) (addr)) = value;
}


#endif
