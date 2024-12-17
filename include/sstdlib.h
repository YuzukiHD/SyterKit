/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Convert a string to an unsigned long integer, with optional base detection.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted unsigned long integer value.
 */
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string to an unsigned long long integer, with optional base detection.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted unsigned long long integer value.
 */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string in hexadecimal format to an unsigned long integer.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @return The converted unsigned long integer value.
 */
unsigned long simple_hextoul(const char *cp, char **endp);

/**
 * Convert a string in decimal format to an unsigned long integer.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @return The converted unsigned long integer value.
 */
unsigned long simple_dectoul(const char *cp, char **endp);

/**
 * Convert a string to a long integer, with optional base detection.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted long integer value.
 */
long simple_strtol(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string to an unsigned long integer, with optional base detection, for unsigned strings.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted unsigned long integer value.
 */
unsigned long simple_ustrtoul(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string to an unsigned long long integer, with optional base detection, for unsigned strings.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted unsigned long long integer value.
 */
unsigned long long simple_ustrtoull(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string to a long long integer, with optional base detection.
 *
 * @param cp The input string.
 * @param endp A pointer to the character after the last parsed character.
 * @param base The number base to use for conversion.
 * @return The converted long long integer value.
 */
long long simple_strtoll(const char *cp, char **endp, unsigned int base);

/**
 * Convert a string to a long integer, with trailing string checking.
 *
 * @param str The input string.
 * @param end The trailing string to check for.
 * @param endp A pointer to the character after the last parsed character.
 * @return The converted long integer value.
 */
long trailing_strtoln_end(const char *str, const char *end, char const **endp);

/**
 * Convert a string to a long integer, with trailing string checking.
 *
 * @param str The input string.
 * @param end The trailing string to check for.
 * @return The converted long integer value.
 */
long trailing_strtoln(const char *str, const char *end);

/**
 * Convert a string to a long integer.
 *
 * @param str The input string.
 * @return The converted long integer value.
 */
long trailing_strtol(const char *str);

/**
 * Convert a string to uppercase.
 *
 * @param in The input string.
 * @param out The output string to store the uppercase characters.
 * @param len The length of the input string.
 */
void str_to_upper(const char *in, char *out, size_t len);

/**
 * Convert a long integer to a string representation.
 *
 * @param num The input long integer value.
 * @param str The output string to store the converted value.
 * @param base The number base to use for conversion.
 * @return A pointer to the output string.
 */
char *ltoa(long int num, char *str, int base);

/**
 * Convert a string to an integer.
 *
 * @param nptr The input string.
 * @return The converted integer value.
 */
int simple_atoi(const char *nptr);

/**
 * Convert a string to a long long integer.
 *
 * @param nptr The input string.
 * @return The converted long long integer value.
 */
long long simple_atoll(const char *nptr);

/**
 * Calculate the absolute value of an integer.
 *
 * @param n The input integer.
 * @return The absolute value of the input integer.
 */
int simple_abs(int n);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __STDLIB_H__