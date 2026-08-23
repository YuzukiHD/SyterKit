/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

typedef union {
	uint64_t value;
	struct {
		uint32_t lo;
		uint32_t hi;
	} word;
} u64_parts_t;

static int parts_compare(u64_parts_t left, u64_parts_t right) {
	if (left.word.hi != right.word.hi)
		return left.word.hi > right.word.hi ? 1 : -1;
	if (left.word.lo != right.word.lo)
		return left.word.lo > right.word.lo ? 1 : -1;
	return 0;
}

static u64_parts_t parts_subtract(u64_parts_t left, u64_parts_t right) {
	u64_parts_t result;

	result.word.lo = left.word.lo - right.word.lo;
	result.word.hi = left.word.hi - right.word.hi - (left.word.lo < right.word.lo);
	return result;
}

static u64_parts_t parts_shift_left_one(u64_parts_t value) {
	value.word.hi = (value.word.hi << 1) | (value.word.lo >> 31);
	value.word.lo <<= 1;
	return value;
}

static uint32_t parts_get_bit(u64_parts_t value, unsigned int bit) {
	if (bit < 32)
		return (value.word.lo >> bit) & 1U;
	return (value.word.hi >> (bit - 32)) & 1U;
}

static void parts_set_bit(u64_parts_t *value, unsigned int bit) {
	if (bit < 32)
		value->word.lo |= 1U << bit;
	else
		value->word.hi |= 1U << (bit - 32);
}

static u64_parts_t udivmod64(u64_parts_t dividend, u64_parts_t divisor, u64_parts_t *remainder_out) {
	u64_parts_t quotient = {.value = 0};
	u64_parts_t remainder = {.value = 0};

	if (divisor.word.hi == 0 && divisor.word.lo == 0)
		return quotient;

	for (int bit = 63; bit >= 0; bit--) {
		remainder = parts_shift_left_one(remainder);
		remainder.word.lo |= parts_get_bit(dividend, bit);
		if (parts_compare(remainder, divisor) >= 0) {
			remainder = parts_subtract(remainder, divisor);
			parts_set_bit(&quotient, bit);
		}
	}

	if (remainder_out)
		*remainder_out = remainder;
	return quotient;
}

uint64_t __udivdi3(uint64_t dividend, uint64_t divisor) {
	u64_parts_t left = {.value = dividend};
	u64_parts_t right = {.value = divisor};

	return udivmod64(left, right, 0).value;
}

uint64_t __umoddi3(uint64_t dividend, uint64_t divisor) {
	u64_parts_t left = {.value = dividend};
	u64_parts_t right = {.value = divisor};
	u64_parts_t remainder;

	udivmod64(left, right, &remainder);
	return remainder.value;
}

uint64_t __ashldi3(uint64_t value, int shift) {
	u64_parts_t input = {.value = value};
	u64_parts_t result = {.value = 0};

	if (shift <= 0)
		return value;
	if (shift < 32) {
		result.word.hi = (input.word.hi << shift) | (input.word.lo >> (32 - shift));
		result.word.lo = input.word.lo << shift;
	} else if (shift < 64) {
		result.word.hi = input.word.lo << (shift - 32);
	}

	return result.value;
}

uint32_t __thead_uread4(const void *address) {
	const volatile uint8_t *bytes = address;
	uint32_t value;

	value = bytes[0];
	value |= (uint32_t) bytes[1] << 8;
	value |= (uint32_t) bytes[2] << 16;
	value |= (uint32_t) bytes[3] << 24;
	return value;
}
