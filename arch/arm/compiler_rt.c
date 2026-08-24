/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file compiler_rt.c
 * @brief Freestanding ARM compiler-runtime arithmetic helpers.
 *
 * These routines provide the 64-bit division, population-count, and floating
 * point conversion entry points emitted by GCC when the target runtime is not
 * linked.  They use integer operations only so they are safe during early
 * boot.
 */

#include <limits.h>
#include <stdint.h>

/** @brief Split representation of an unsigned 64-bit value. */
typedef union {
	uint64_t value;
	struct {
		uint32_t lo;
		uint32_t hi;
	} word;
} u64_parts_t;

/**
 * @brief Compare two split 64-bit values without using native division.
 * @param[in] left First value.
 * @param[in] right Second value.
 * @return -1, zero, or one according to the unsigned ordering.
 */
static int parts_compare(u64_parts_t left, u64_parts_t right)
{
	if (left.word.hi != right.word.hi)
		return left.word.hi > right.word.hi ? 1 : -1;
	if (left.word.lo != right.word.lo)
		return left.word.lo > right.word.lo ? 1 : -1;
	return 0;
}

/**
 * @brief Subtract one split 64-bit value from another.
 * @param[in] left Minuend.
 * @param[in] right Subtrahend.
 * @return Difference represented as split words.
 */
static u64_parts_t parts_subtract(u64_parts_t left, u64_parts_t right)
{
	u64_parts_t result;

	result.word.lo = left.word.lo - right.word.lo;
	result.word.hi = left.word.hi - right.word.hi - (left.word.lo < right.word.lo);
	return result;
}

/**
 * @brief Shift a split 64-bit value left by one bit.
 * @param[in] value Value to shift.
 * @return Shifted value.
 */
static u64_parts_t parts_shift_left_one(u64_parts_t value)
{
	value.word.hi = (value.word.hi << 1) | (value.word.lo >> 31);
	value.word.lo <<= 1;
	return value;
}

/**
 * @brief Read one bit from a split 64-bit value.
 * @param[in] value Value to inspect.
 * @param[in] bit Bit index in the range 0 through 63.
 * @return Zero or one for the selected bit.
 */
static uint32_t parts_get_bit(u64_parts_t value, unsigned int bit)
{
	if (bit < 32)
		return (value.word.lo >> bit) & 1U;
	return (value.word.hi >> (bit - 32)) & 1U;
}

/**
 * @brief Set one bit in a split 64-bit value.
 * @param[in,out] value Value to modify.
 * @param[in] bit Bit index in the range 0 through 63.
 */
static void parts_set_bit(u64_parts_t *value, unsigned int bit)
{
	if (bit < 32)
		value->word.lo |= 1U << bit;
	else
		value->word.hi |= 1U << (bit - 32);
}

/**
 * @brief Divide two split 64-bit values with a restoring binary division.
 * @param[in] dividend Value to divide.
 * @param[in] divisor Non-zero divisor; zero returns a zero quotient.
 * @param[out] remainder_out Optional remainder destination.
 * @return Quotient represented as split words.
 */
static u64_parts_t udivmod64(u64_parts_t dividend, u64_parts_t divisor, u64_parts_t *remainder_out)
{
	u64_parts_t quotient = { .value = 0 };
	u64_parts_t remainder = { .value = 0 };

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

/**
 * @brief GCC ABI entry point for unsigned 64-bit division and remainder.
 * @param[in] dividend Dividend.
 * @param[in] divisor Divisor; zero produces a zero quotient and remainder.
 * @param[out] remainder_out Optional remainder destination.
 * @return Unsigned quotient.
 */
uint64_t __udivmoddi4(uint64_t dividend, uint64_t divisor, uint64_t *remainder_out)
{
	u64_parts_t left = { .value = dividend };
	u64_parts_t right = { .value = divisor };
	u64_parts_t remainder = { .value = 0 };
	u64_parts_t quotient = udivmod64(left, right, &remainder);

	if (remainder_out)
		*remainder_out = remainder.value;
	return quotient.value;
}

/**
 * @brief GCC ABI entry point counting set bits in a 32-bit value.
 * @param[in] value Value whose bits are counted.
 * @return Number of one bits in @p value.
 */
int __popcountsi2(uint32_t value)
{
	value -= (value >> 1) & 0x55555555U;
	value = (value & 0x33333333U) + ((value >> 2) & 0x33333333U);
	value = (value + (value >> 4)) & 0x0f0f0f0fU;
	value += value >> 8;
	value += value >> 16;
	return value & 0x3fU;
}

/**
 * @brief Convert the magnitude of a double to an unsigned 64-bit integer.
 * @param[in] value IEEE-754 value to inspect.
 * @param[out] negative Receives one when the source sign bit is set.
 * @return Truncated magnitude, saturated at `UINT64_MAX` on overflow.
 */
static uint64_t double_magnitude(double value, int *negative)
{
	union {
		double value;
		struct {
			uint32_t lo;
			uint32_t hi;
		} word;
	} input = { .value = value };
	u64_parts_t result = { .value = 0 };
	uint32_t exponent = (input.word.hi >> 20) & 0x7ffU;
	uint32_t shift;

	*negative = input.word.hi >> 31;
	if (exponent < 1023U)
		return 0;
	if (exponent >= 1087U)
		return UINT64_MAX;

	input.word.hi = (input.word.hi & 0x000fffffU) | 0x00100000U;
	shift = exponent - 1023U;
	if (shift < 52U) {
		uint32_t right = 52U - shift;

		if (right < 32U) {
			result.word.lo = (input.word.lo >> right) | (input.word.hi << (32U - right));
			result.word.hi = input.word.hi >> right;
		} else {
			result.word.lo = input.word.hi >> (right - 32U);
		}
	} else {
		uint32_t left = shift - 52U;

		if (left == 0) {
			result.word.lo = input.word.lo;
			result.word.hi = input.word.hi;
		} else if (left < 32U) {
			result.word.hi = (input.word.hi << left) | (input.word.lo >> (32U - left));
			result.word.lo = input.word.lo << left;
		} else {
			result.word.hi = input.word.lo << (left - 32U);
		}
	}

	return result.value;
}

/**
 * @brief GCC ABI conversion from double to unsigned 64-bit integer.
 * @param[in] value IEEE-754 value to convert.
 * @return Truncated non-negative value, or zero for negative input.
 */
uint64_t __fixunsdfdi(double value)
{
	int negative;
	uint64_t magnitude = double_magnitude(value, &negative);

	return negative ? 0 : magnitude;
}

/**
 * @brief GCC ABI conversion from double to signed 64-bit integer.
 * @param[in] value IEEE-754 value to convert.
 * @return Truncated value saturated to the signed 64-bit range.
 */
int64_t __fixdfdi(double value)
{
	int negative;
	u64_parts_t magnitude = { .value = double_magnitude(value, &negative) };

	if (!negative) {
		if (magnitude.word.hi >= 0x80000000U)
			return INT64_MAX;
		return (int64_t)magnitude.value;
	}
	if (magnitude.word.hi >= 0x80000000U)
		return INT64_MIN;

	magnitude.word.lo = ~magnitude.word.lo + 1U;
	magnitude.word.hi = ~magnitude.word.hi + (magnitude.word.lo == 0);
	return (int64_t)magnitude.value;
}

uint64_t __aeabi_d2ulz(double value) __attribute__((alias("__fixunsdfdi")));
int64_t __aeabi_d2lz(double value) __attribute__((alias("__fixdfdi")));
