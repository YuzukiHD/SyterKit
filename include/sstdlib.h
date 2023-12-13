/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

extern unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

extern unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base);

extern unsigned long simple_hextoul(const char *cp, char **endp);

extern unsigned long simple_dectoul(const char *cp, char **endp);

extern long simple_strtol(const char *cp, char **endp, unsigned int base);

extern unsigned long simple_ustrtoul(const char *cp, char **endp, unsigned int base);

extern unsigned long long simple_ustrtoull(const char *cp, char **endp, unsigned int base);

extern long long simple_strtoll(const char *cp, char **endp, unsigned int base);

extern long trailing_strtoln_end(const char *str, const char *end, char const **endp);

extern long trailing_strtoln(const char *str, const char *end);

extern long trailing_strtol(const char *str);

extern void str_to_upper(const char *in, char *out, size_t len);

extern char *ltoa(long int num, char *str, int base);

extern int simple_atoi(const char *nptr);

extern long long simple_atoll(const char *nptr);

extern int simple_abs(int n);


#endif// __STDLIB_H__