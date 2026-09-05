/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file ctype.c
 * @brief Character classification and conversion helpers.
 *
 * These routines implement the standard C <ctype.h> predicates and the
 * ASCII case-conversion helpers used throughout the firmware.  The
 * comparisons operate on the value of the character argument after casting
 * to unsigned, so the behaviour is well-defined for the full range of int.
 */
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

/**
 * @brief Check whether a character is alphabetic.
 *
 * @param[in] c Character to test, cast to unsigned char for the comparison.
 * @return Non-zero if @p c is a letter, zero otherwise.
 */
int isalpha(int c)
{
	return (((unsigned)c | 32) - 'a') < 26;
}

/**
 * @brief Check whether a character is a valid 7-bit ASCII value.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is in the range 0x00..0x7f, zero otherwise.
 */
int isascii(int c)
{
	return !(c & ~0x7f);
}

/**
 * @brief Check whether a character is a blank (space or horizontal tab).
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a space or a horizontal tab, zero otherwise.
 */
int isblank(int c)
{
	return (c == ' ' || c == '\t');
}

/**
 * @brief Check whether a character is alphanumeric.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a letter or a digit, zero otherwise.
 */
int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

/**
 * @brief Check whether a character is a control character.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a control character, zero otherwise.
 */
int iscntrl(int c)
{
	return ((unsigned)c < 0x20) || (c == 0x7f);
}

/**
 * @brief Check whether a character is a decimal digit.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a decimal digit, zero otherwise.
 */
int isdigit(int c)
{
	return ((unsigned)c - '0') < 10;
}

/**
 * @brief Check whether a character is a printable, non-space character.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c has a graphical representation, zero otherwise.
 */
int isgraph(int c)
{
	return ((unsigned)c - 0x21) < 0x5e;
}

/**
 * @brief Check whether a character is a lowercase letter.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a lowercase letter, zero otherwise.
 */
int islower(int c)
{
	return ((unsigned)c - 'a') < 26;
}

/**
 * @brief Check whether a character is printable, including space.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is printable, zero otherwise.
 */
int isprint(int c)
{
	return ((unsigned)c - 0x20) < 0x5f;
}

/**
 * @brief Check whether a character is punctuation.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a printable character that is neither a
 *         letter nor a digit, zero otherwise.
 */
int ispunct(int c)
{
	return isgraph(c) && !isalnum(c);
}

/**
 * @brief Check whether a character is white space.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a space, tab, or one of the other standard
 *         white-space characters, zero otherwise.
 */
int isspace(int c)
{
	return (c == ' ') || ((unsigned)c - '\t' < 5);
}

/**
 * @brief Check whether a character is an uppercase letter.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is an uppercase letter, zero otherwise.
 */
int isupper(int c)
{
	return ((unsigned)c - 'A') < 26;
}

/**
 * @brief Check whether a character is a hexadecimal digit.
 *
 * @param[in] c Character to test.
 * @return Non-zero if @p c is a decimal digit or a hexadecimal letter
 *         (a-f or A-F), zero otherwise.
 */
int isxdigit(int c)
{
	return isdigit(c) || (((unsigned)c | 32) - 'a' < 6);
}

/**
 * @brief Reduce a character to its 7-bit ASCII value.
 *
 * @param[in] c Character to convert.
 * @return @p c masked to the lower seven bits.
 */
int toascii(int c)
{
	return (c & 0x7f);
}

/**
 * @brief Convert an uppercase letter to lowercase.
 *
 * @param[in] c Character to convert.
 * @return The lowercase equivalent of @p c if it is an uppercase letter,
 *         otherwise @p c unchanged.
 */
int tolower(int c)
{
	if (isupper(c))
		return c | 32;
	return c;
}

/**
 * @brief Convert a lowercase letter to uppercase.
 *
 * @param[in] c Character to convert.
 * @return The uppercase equivalent of @p c if it is a lowercase letter,
 *         otherwise @p c unchanged.
 */
int toupper(int c)
{
	if (islower(c))
		return c & 0x5f;
	return c;
}