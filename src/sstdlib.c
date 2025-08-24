/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "ctype.h"
#include "sstdlib.h"

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

static unsigned int decode_digit(int ch) {
	if (!isxdigit(ch))
		return 256;

	ch = tolower(ch);

	return ch <= '9' ? ch - '0' : ch - 'a' + 0xa;
}

int simple_abs(int n) {
	return ((n < 0) ? -n : n);
}

int simple_atoi(const char *nptr) {
	return (int) simple_strtol(nptr, NULL, 10);
}

long long simple_atoll(const char *nptr) {
	return (long long) simple_strtoll(nptr, NULL, 10);
}

long simple_strtol(const char *cp, char **endp, unsigned int base) {
	if (*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);

	return simple_strtoul(cp, endp, base);
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

unsigned long simple_hextoul(const char *cp, char **endp) {
	return simple_strtoul(cp, endp, 16);
}

unsigned long simple_dectoul(const char *cp, char **endp) {
	return simple_strtoul(cp, endp, 10);
}

long strtol(const char *cp, char **endp, unsigned int base) {
	if (*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);

	return simple_strtoul(cp, endp, base);
}

unsigned long simple_ustrtoul(const char *cp, char **endp, unsigned int base) {
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

unsigned long long simple_ustrtoull(const char *cp, char **endp, unsigned int base) {
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

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base) {
	unsigned long long result = 0;
	unsigned int value;

	cp = _parse_integer_fixup_radix(cp, &base);

	while (value = decode_digit(*cp), value < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *) cp;

	return result;
}

long long simple_strtoll(const char *cp, char **endp, unsigned int base) {
	if (*cp == '-')
		return -simple_strtoull(cp + 1, endp, base);

	return simple_strtoull(cp, endp, base);
}

long trailing_strtoln_end(const char *str, const char *end, char const **endp) {
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

long trailing_strtoln(const char *str, const char *end) {
	return trailing_strtoln_end(str, end, NULL);
}

long trailing_strtol(const char *str) {
	return trailing_strtoln(str, NULL);
}

void str_to_upper(const char *in, char *out, size_t len) {
	for (; len > 0 && *in; len--) *out++ = toupper(*in++);
	if (len)
		*out = '\0';
}

char *ltoa(long int num, char *str, int base) {
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
