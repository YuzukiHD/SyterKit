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

void *memchr(void *src, int val, unsigned int cnt) {
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

static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base) {
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

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base) {
    unsigned long long result = 0, value;

    cp = _parse_integer_fixup_radix(cp, &base);

    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0'
                                                  : (islower(*cp) ? toupper(*cp) : *cp) - 'A' + 10) < base) {
        result = result * base + value;
        cp++;
    }

    if (endp)
        *endp = (char *) cp;

    return result;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
    unsigned long result = 0;
    unsigned long value;

    cp = _parse_integer_fixup_radix(cp, &base);

    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0' : (islower(*cp) ? toupper(*cp) : *cp) - 'A' + 10) < base) {
        result = result * base + value;
        cp++;
    }

    if (endp)
        *endp = (char *) cp;

    return result;
}