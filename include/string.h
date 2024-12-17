/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STRING_H__
#define __STRING_H__

#include <stdarg.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Copies the values of 'cnt' bytes from memory area 'src' to memory area 'dst'.
 *
 * @param dst The destination memory area.
 * @param src The source memory area.
 * @param cnt The number of bytes to copy.
 * @return A pointer to the destination memory area.
 */
void *memcpy(void *dst, const void *src, int cnt) __attribute__((optimize("O0")));

/**
 * Sets the first 'cnt' bytes of the memory area pointed to by 'dst' to the specified value 'val'.
 *
 * @param dst The memory area to be filled.
 * @param val The value to be set.
 * @param cnt The number of bytes to be set to the value.
 * @return A pointer to the destination memory area.
 */
void *memset(void *dst, int val, int cnt) __attribute__((optimize("O0")));

/**
 * Compares the first 'cnt' bytes of the memory areas 'dst' and 'src'.
 *
 * @param dst The first memory area to compare.
 * @param src The second memory area to compare.
 * @param cnt The number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if 'dst' is found, respectively, to be less than, to match, or be greater than 'src'.
 */
int memcmp(const void *dst, const void *src, unsigned int cnt) __attribute__((optimize("O0")));

/**
 * Calculates the length of the string 'str', excluding the terminating null byte.
 *
 * @param str The input string.
 * @return The length of the input string.
 */
unsigned int strlen(const char *str) __attribute__((optimize("O0")));

/**
 * Calculates the length of the string 's', but not more than 'n' characters.
 *
 * @param s The input string.
 * @param n The maximum number of characters to examine.
 * @return The length of the input string, but not more than 'n'.
 */
unsigned int strnlen(const char *s, unsigned int n) __attribute__((optimize("O0")));

/**
 * Copies the string pointed to by 'src', including the terminating null byte, to the buffer pointed to by 'dst'.
 *
 * @param dst The destination buffer.
 * @param src The source string.
 * @return A pointer to the destination buffer.
 */
char *strcpy(char *dst, const char *src) __attribute__((optimize("O0")));

/**
 * Copies up to 'n' characters from the string pointed to by 'src', including the terminating null byte, to the buffer pointed to by 'dest'.
 *
 * @param dest The destination buffer.
 * @param src The source string.
 * @param n The maximum number of characters to be copied.
 * @return A pointer to the destination buffer.
 */
char *strncpy(char *dest, const char *src, unsigned int n) __attribute__((optimize("O0")));

/**
 * Appends the string pointed to by 'src', including the terminating null byte, to the end of the string pointed to by 'dst'.
 *
 * @param dst The destination string.
 * @param src The source string.
 * @return A pointer to the destination string.
 */
char *strcat(char *dst, const char *src) __attribute__((optimize("O0")));

/**
 * Compares the string pointed to by 'p1' to the string pointed to by 'p2'.
 *
 * @param p1 The first string to compare.
 * @param p2 The second string to compare.
 * @return An integer less than, equal to, or greater than zero if 'p1' is found, respectively, to be less than, to match, or be greater than 'p2'.
 */
int strcmp(const char *p1, const char *p2) __attribute__((optimize("O0")));

/**
 * Compares at most the first 'cnt' characters of the string pointed to by 'p1' to the string pointed to by 'p2'.
 *
 * @param p1 The first string to compare.
 * @param p2 The second string to compare.
 * @param cnt The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if 'p1' is found, respectively, to be less than, to match, or be greater than 'p2'.
 */
int strncmp(const char *p1, const char *p2, unsigned int cnt) __attribute__((optimize("O0")));

/**
 * Finds the first occurrence of the character 'c' (converted to a char) in the string pointed to by 's', including the terminating null byte.
 *
 * @param s The input string.
 * @param c The character to search for.
 * @return A pointer to the located character, or NULL if the character does not occur in the string.
 */
char *strchr(const char *s, int c) __attribute__((optimize("O0")));

/**
 * Finds the last occurrence of the character 'c' (converted to a char) in the string pointed to by 's', including the terminating null byte.
 *
 * @param s The input string.
 * @param c The character to search for.
 * @return A pointer to the located character, or NULL if the character does not occur in the string.
 */
char *strrchr(const char *s, int c) __attribute__((optimize("O0")));

/**
 * Finds the first occurrence of the substring 'what' in the string 's'.
 *
 * @param s The input string.
 * @param what The substring to search for.
 * @return A pointer to the beginning of the located substring, or NULL if the substring does not occur in the string.
 */
char *strstr(const char *s, const char *what) __attribute__((optimize("O0")));

/**
 * Locates the first occurrence of the character 'value' (converted to an unsigned char) in the first 'num' bytes of the memory area pointed to by 'ptr'.
 *
 * @param ptr The input memory area.
 * @param value The value to search for.
 * @param num The number of bytes to examine.
 * @return A pointer to the located character, or NULL if the character does not occur in the memory area.
 */
void *memchr(void *ptr, int value, unsigned int num) __attribute__((optimize("O0")));

/**
 * Copies 'count' bytes from the memory area 'src' to the memory area 'dest'. The memory areas may overlap.
 *
 * @param dest The destination memory area.
 * @param src The source memory area.
 * @param count The number of bytes to copy.
 * @return A pointer to the destination memory area.
 */
void *memmove(void *dest, const void *src, unsigned int count) __attribute__((optimize("O0")));

#ifdef CONFIG_SPRINTF
/**
 * @brief Writes formatted data to a string.
 * @param buf Pointer to the destination string where the formatted data will be stored.
 * @param fmt Format string containing the data formats to be written.
 * @param ... Variable number of arguments to be formatted and written into buf according to fmt.
 * @return Returns the number of characters written to buf (excluding the null terminator).
 */
int sprintf(char *buf, const char *fmt, ...);

/**
 * @brief Safely writes formatted data to a string, limiting the number of characters written.
 * @param buf Pointer to the destination string where the formatted data will be stored.
 * @param n Maximum number of characters allowed to be written (including the null terminator).
 * @param fmt Format string containing the data formats to be written.
 * @param ... Variable number of arguments to be formatted and written into buf according to fmt. 
 *            It will not exceed n-1 characters, and a null terminator will be added at the end.
 * @return Returns the number of characters written to buf (excluding the null terminator).
 *         If the output is truncated, it returns the total number of characters attempted to be written.
 */
int snprintf(char *buf, size_t n, const char *fmt, ...);

/**
 * @brief Similar to snprintf, but takes a va_list type parameter list for handling variable arguments.
 * @param buf Pointer to the destination string where the formatted data will be stored.
 * @param n Maximum number of characters allowed to be written (including the null terminator).
 * @param fmt Format string containing the data formats to be written.
 * @param ap va_list type parameter list for passing variable number of arguments.
 * @return Returns the number of characters written to buf (excluding the null terminator).
 *         If the output is truncated, it returns the total number of characters attempted to be written.
 */
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

#endif// CONFIG_SPRINTF

#ifdef __cplusplus
}
#endif// __cplusplus

#endif /* #ifndef __STRING_H__ */
