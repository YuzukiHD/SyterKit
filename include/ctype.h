/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CTYPE_H__
#define __CTYPE_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Check if the given character is alphanumeric.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is alphanumeric, zero otherwise.
 */
int isalnum(int c);

/**
 * Check if the given character is alphabetic.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is alphabetic, zero otherwise.
 */
int isalpha(int c);

/**
 * Check if the given character is a valid ASCII character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a valid ASCII character, zero otherwise.
 */
int isascii(int c);

/**
 * Check if the given character is a blank space.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a blank space, zero otherwise.
 */
int isblank(int c);

/**
 * Check if the given character is a control character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a control character, zero otherwise.
 */
int iscntrl(int c);

/**
 * Check if the given character is a digit.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a digit, zero otherwise.
 */
int isdigit(int c);

/**
 * Check if the given character is printable and not a space.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is printable and not a space, zero otherwise.
 */
int isgraph(int c);

/**
 * Check if the given character is a lowercase alphabetic character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a lowercase alphabetic character, zero otherwise.
 */
int islower(int c);

/**
 * Check if the given character is a printable character (including spaces).
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a printable character, zero otherwise.
 */
int isprint(int c);

/**
 * Check if the given character is a punctuation character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a punctuation character, zero otherwise.
 */
int ispunct(int c);

/**
 * Check if the given character is a whitespace character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a whitespace character, zero otherwise.
 */
int isspace(int c);

/**
 * Check if the given character is an uppercase alphabetic character.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is an uppercase alphabetic character, zero otherwise.
 */
int isupper(int c);

/**
 * Check if the given character is a hexadecimal digit.
 *
 * @param c The character to check.
 * @return Non-zero value if the character is a hexadecimal digit, zero otherwise.
 */
int isxdigit(int c);

/**
 * Convert the given character to its ASCII equivalent.
 *
 * @param c The character to convert.
 * @return The ASCII value of the character.
 */
int toascii(int c);

/**
 * Convert the given character to lowercase.
 *
 * @param c The character to convert.
 * @return The lowercase equivalent of the character.
 */
int tolower(int c);

/**
 * Convert the given character to uppercase.
 *
 * @param c The character to convert.
 * @return The uppercase equivalent of the character.
 */
int toupper(int c);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif /* __CTYPE_H__ */