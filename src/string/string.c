/* SPDX-License-Identifier: MIT */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "string.h"

unsigned int strlen(const char *str) {
    int i = 0;

    while (str[i++] != '\0')
        ;

    return i - 1;
}

unsigned int strnlen(const char *s, unsigned int n) {
    const char *sc;

    for (sc = s; n-- && *sc != '\0'; ++sc)
        ;
    return sc - s;
}

char *strcpy(char *dst, const char *src) {
    char *bak = dst;

    while ((*dst++ = *src++) != '\0')
        ;

    return bak;
}

char *strcat(char *dst, const char *src) {
    char *p = dst;

    while (*dst != '\0')
        dst++;

    while ((*dst++ = *src++) != '\0')
        ;

    return p;
}

int strcmp(const char *p1, const char *p2) {
    unsigned char c1, c2;

    while (1) {
        c1 = *p1++;
        c2 = *p2++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }

    return 0;
}

int strncmp(const char *p1, const char *p2, unsigned int cnt) {
    unsigned char c1, c2;

    while (cnt--) {
        c1 = *p1++;
        c2 = *p2++;

        if (c1 != c2)
            return c1 < c2 ? -1 : 1;

        if (!c1)
            break;
    }

    return 0;
}

char *strchr(const char *s, int c) {
    for (; *s != (char) c; ++s)
        if (*s == '\0')
            return NULL;

    return (char *) s;
}

char *strrchr(const char *s, int c) {
    const char *p = s + strlen(s);

    do {
        if (*p == (char) c)
            return (char *) p;
    } while (--p >= s);

    return NULL;
}

char *strstr(const char *s1, const char *s2) {
    register const char *s = s1;
    register const char *p = s2;

    do {
        if (!*p) {
            return (char *) s1;
            ;
        }
        if (*p == *s) {
            ++p;
            ++s;
        } else {
            p = s2;
            if (!*s) {
                return NULL;
            }
            s = ++s1;
        }
    } while (1);
}

void *memchr(const char *src, int val, unsigned int cnt) {
    char *p = NULL;
    char *s = (char *) src;

    while (cnt) {
        if (*s == val) {
            p = s;
            break;
        }
        s++;
        cnt--;
    }

    return p;
}

void *memmove(void *dst, const void *src, unsigned int cnt) {
    char *p, *s;

    if (dst <= src) {
        p = (char *) dst;
        s = (char *) src;
        while (cnt--)
            *p++ = *s++;
    } else {
        p = (char *) dst + cnt;
        s = (char *) src + cnt;
        while (cnt--)
            *--p = *--s;
    }

    return dst;
}
