/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file stdlib.c
 * @brief Integer string conversions and related helpers.
 *
 * This file provides the simple_* family of U-Boot-style number parsers
 * together with a few convenience wrappers and the trailing-number
 * helpers used for parsing kernel and filesystem version suffixes.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "ctype.h"
#include "stdlib.h"

/**
 * @brief Resolve an unspecified radix from the leading string characters.
 *
 * When the caller passes base 0 the radix is deduced from the prefix: 0x
 * selects hexadecimal, a leading zero selects octal, and anything else
 * selects decimal.  A hexadecimal 0x prefix is skipped in the returned
 * pointer.
 *
 * @param[in] s Pointer to the start of the digit string.
 * @param[in,out] base Radix supplied by the caller; replaced with the
 *             deduced radix when the caller passes 0.
 * @return Pointer to the first character to parse, after any 0x prefix.
 */
static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && tolower(s[1]) == 'x')
		s += 2;
	return s;
}

/**
 * @brief Convert a hexadecimal digit character to its numeric value.
 *
 * @param[in] ch Character to decode.
 * @return The value of the digit, or 256 when @p ch is not a hexadecimal
 *         digit.
 */
static unsigned int decode_digit(int ch)
{
	if (!isxdigit(ch))
		return 256;

	ch = tolower(ch);

	return ch <= '9' ? ch - '0' : ch - 'a' + 0xa;
}

/**
 * @brief Compute the absolute value of an integer.
 *
 * @param[in] n Value to evaluate.
 * @return The absolute value of @p n.
 */
int simple_abs(int n)
{
	return ((n < 0) ? -n : n);
}

/**
 * @brief Parse a decimal integer from a string.
 *
 * @param[in] nptr Null-terminated string to parse.
 * @return The parsed value as an int.
 */
int simple_atoi(const char *nptr)
{
	return (int)simple_strtol(nptr, NULL, 10);
}

/**
 * @brief Parse a decimal integer from a string into a long long.
 *
 * @param[in] nptr Null-terminated string to parse.
 * @return The parsed value as a long long.
 */
long long simple_atoll(const char *nptr)
{
	return (long long)simple_strtoll(nptr, NULL, 10);
}

/**
 * @brief Parse a signed integer with an explicit base.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The parsed value as a long.
 */
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	if (*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);

	return simple_strtoul(cp, endp, base);
}

/**
 * @brief Parse an unsigned integer with an explicit base.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The parsed value as an unsigned long.
 */
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	cp = _parse_integer_fixup_radix(cp, &base);

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0' : (islower(*cp) ? toupper(*cp) : *cp) - 'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

/**
 * @brief Parse a hexadecimal integer from a string.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @return The parsed value as an unsigned long.
 */
unsigned long simple_hextoul(const char *cp, char **endp)
{
	return simple_strtoul(cp, endp, 16);
}

/**
 * @brief Parse a decimal integer from a string.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @return The parsed value as an unsigned long.
 */
unsigned long simple_dectoul(const char *cp, char **endp)
{
	return simple_strtoul(cp, endp, 10);
}

/**
 * @brief Standard strtol()-style signed integer parser.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The parsed value as a long.
 */
long strtol(const char *cp, char **endp, unsigned int base)
{
	if (*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);

	return simple_strtoul(cp, endp, base);
}

/**
 * @brief Parse an integer honouring binary magnitude suffixes.
 *
 * A trailing g, m, or k (case-insensitive, optionally followed by "i" and
 * "B") scales the parsed value by the corresponding power of 1024.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The scaled value as an unsigned long.
 */
unsigned long simple_ustrtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = simple_strtoul(cp, endp, base);
	switch (tolower(**endp)) {
	case 'g':
		result *= 1024;
		/* fall through */
	case 'm':
		result *= 1024;
		/* fall through */
	case 'k':
		result *= 1024;
		(*endp)++;
		if (**endp == 'i')
			(*endp)++;
		if (**endp == 'B')
			(*endp)++;
	}
	return result;
}

/**
 * @brief Parse a long long honouring binary magnitude suffixes.
 *
 * A trailing g, m, or k (case-insensitive, optionally followed by "i" and
 * "B") scales the parsed value by the corresponding power of 1024.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The scaled value as an unsigned long long.
 */
unsigned long long simple_ustrtoull(const char *cp, char **endp, unsigned int base)
{
	unsigned long long result = simple_strtoull(cp, endp, base);
	switch (tolower(**endp)) {
	case 'g':
		result *= 1024;
		/* fall through */
	case 'm':
		result *= 1024;
		/* fall through */
	case 'k':
		result *= 1024;
		(*endp)++;
		if (**endp == 'i')
			(*endp)++;
		if (**endp == 'B')
			(*endp)++;
	}
	return result;
}

/**
 * @brief Parse an unsigned long long with an explicit base.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The parsed value as an unsigned long long.
 */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
	unsigned long long result = 0;
	unsigned int value;

	cp = _parse_integer_fixup_radix(cp, &base);

	while (value = decode_digit(*cp), value < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

/**
 * @brief Parse a signed long long with an explicit base.
 *
 * @param[in] cp Null-terminated string to parse.
 * @param[out] endp Optional pointer that receives the address of the first
 *             unparsed character, or NULL if not needed.
 * @param[in] base Radix to use; 0 selects the prefix-based radix.
 * @return The parsed value as a long long.
 */
long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
	if (*cp == '-')
		return -simple_strtoull(cp + 1, endp, base);

	return simple_strtoull(cp, endp, base);
}

/**
 * @brief Parse the trailing run of digits at the end of a string.
 *
 * Scans backwards from @p end to find the last digit run and parses it as
 * a decimal integer.  The position of the first digit is reported through
 * @p endp.
 *
 * @param[in] str Start of the string to examine.
 * @param[in] end Pointer just past the end of the string, or NULL to use
 *             the string terminator.
 * @param[out] endp Optional pointer that receives the address of the first
 *             digit, or NULL if not needed.
 * @return The parsed decimal value, or -1 when no trailing digit run is
 *         present.
 */
long trailing_strtoln_end(const char *str, const char *end, char const **endp)
{
	const char *p;

	if (!end)
		end = str + strlen(str);
	p = end - 1;
	if (p > str && isdigit(*p)) {
		do {
			if (!isdigit(p[-1])) {
				if (endp)
					*endp = p;
				return simple_dectoul(p, NULL);
			}
		} while (--p > str);
	}
	if (endp)
		*endp = end;

	return -1;
}

/**
 * @brief Parse the trailing digit run within an explicit range.
 *
 * @param[in] str Start of the string to examine.
 * @param[in] end Pointer just past the end of the string, or NULL to use
 *             the string terminator.
 * @return The parsed decimal value, or -1 when no trailing digit run is
 *         present.
 */
long trailing_strtoln(const char *str, const char *end)
{
	return trailing_strtoln_end(str, end, NULL);
}

/**
 * @brief Parse the trailing digit run of a null-terminated string.
 *
 * @param[in] str Null-terminated string to examine.
 * @return The parsed decimal value, or -1 when no trailing digit run is
 *         present.
 */
long trailing_strtol(const char *str)
{
	return trailing_strtoln(str, NULL);
}

/**
 * @brief Copy a string converting every character to uppercase.
 *
 * @param[in] in Null-terminated source string.
 * @param[out] out Destination buffer receiving the uppercased string.
 * @param[in] len Capacity of @p out in bytes; the output is always
 *             null-terminated when room remains.
 */
void str_to_upper(const char *in, char *out, size_t len)
{
	for (; len > 0 && *in; len--)
		*out++ = toupper(*in++);
	if (len)
		*out = '\0';
}

/**
 * @brief Convert a long integer to its string representation.
 *
 * Digits are generated least-significant first and then reversed in place.
 * A leading minus sign is emitted for negative decimal values.
 *
 * @param[in] num Value to convert.
 * @param[out] str Buffer receiving the null-terminated string.
 * @param[in] base Radix to use (2..36).
 * @return A pointer to @p str.
 */
char *ltoa(long int num, char *str, int base)
{
	int i = 0;
	int is_negative = 0;

	if (num < 0 && base == 10) {
		is_negative = 1;
		num = -num;
	}

	do {
		int digit = num % base;
		str[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
		num /= base;
	} while (num > 0);

	if (is_negative) {
		str[i++] = '-';
	}

	str[i] = '\0';

	int len = strlen(str);
	for (int j = 0; j < len / 2; j++) {
		char temp = str[j];
		str[j] = str[len - j - 1];
		str[len - j - 1] = temp;
	}

	return str;
}
