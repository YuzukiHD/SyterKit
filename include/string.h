/* SPDX-License-Identifier: MIT */

#ifndef __STRING_H__
#define __STRING_H__

#include <types.h>

extern void		*memcpy(void *dst, const void *src, int cnt);
extern void		*memset(void *dst, int val, int cnt);
extern int			memcmp(const void *dst, const void *src, unsigned int cnt);
extern unsigned int strlen(const char *str);
extern char		*strcpy(char *dst, const char *src);
extern char		*strcat(char *dst, const char *src);
extern int			strcmp(const char *p1, const char *p2);
extern int			strncmp(const char *p1, const char *p2, unsigned int cnt);
extern char		*strchr(const char *s, int c);
extern char		*strstr(const char *s, const char *what);
extern void		*memchr(void *ptr, int value, unsigned int num);
extern void		*memmove(void *dest, const void *src, unsigned int count);

#endif /* #ifndef __STRING_H__ */
