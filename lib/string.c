/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file string.c
 * @brief String and memory manipulation primitives.
 *
 * This file supplies the small C library string routines used by the
 * firmware, including length, copy, compare, and search helpers.  When
 * CONFIG_SPRINTF is enabled it also provides a compact printf-family
 * implementation with integer and floating-point formatting.
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Compute the length of a null-terminated string.
 *
 * @param[in] str Null-terminated string to measure.
 * @return Number of characters before the terminating null byte.
 */
unsigned int strlen(const char *str)
{
	int i = 0;

	while (str[i++] != '\0')
		;

	return i - 1;
}

/**
 * @brief Compute the bounded length of a string.
 *
 * @param[in] s String to measure.
 * @param[in] n Maximum number of characters to examine.
 * @return The length of @p s, limited to @p n.
 */
unsigned int strnlen(const char *s, unsigned int n)
{
	const char *sc;

	for (sc = s; n-- && *sc != '\0'; ++sc)
		;
	return sc - s;
}

/**
 * @brief Copy a null-terminated string into a destination buffer.
 *
 * @param[out] dst Destination buffer.
 * @param[in] src Source string.
 * @return A pointer to @p dst.
 */
char *strcpy(char *dst, const char *src)
{
	char *bak = dst;

	while ((*dst++ = *src++) != '\0')
		;

	return bak;
}

/**
 * @brief Append a copy of a string to the end of another string.
 *
 * @param[out] dst Destination string to extend.
 * @param[in] src Source string to append.
 * @return A pointer to @p dst.
 */
char *strcat(char *dst, const char *src)
{
	char *p = dst;

	while (*dst != '\0')
		dst++;

	while ((*dst++ = *src++) != '\0')
		;

	return p;
}

/**
 * @brief Compare two null-terminated strings lexicographically.
 *
 * @param[in] p1 First string.
 * @param[in] p2 Second string.
 * @return Negative, zero, or positive as @p p1 is less than, equal to, or
 *         greater than @p p2.
 */
int strcmp(const char *p1, const char *p2)
{
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

/**
 * @brief Compare at most cnt characters of two strings.
 *
 * @param[in] p1 First string.
 * @param[in] p2 Second string.
 * @param[in] cnt Maximum number of characters to compare.
 * @return Negative, zero, or positive as @p p1 is less than, equal to, or
 *         greater than @p p2 within the first @p cnt characters.
 */
int strncmp(const char *p1, const char *p2, unsigned int cnt)
{
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

/**
 * @brief Locate the first occurrence of a character in a string.
 *
 * @param[in] s Null-terminated string to search.
 * @param[in] c Character to find, interpreted as an unsigned char.
 * @return Pointer to the first occurrence of @p c, or NULL if it is not
 *         present.
 */
char *strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *)s;
}

/**
 * @brief Locate the last occurrence of a character in a string.
 *
 * @param[in] s Null-terminated string to search.
 * @param[in] c Character to find, interpreted as an unsigned char.
 * @return Pointer to the last occurrence of @p c, or NULL if it is not
 *         present.
 */
char *strrchr(const char *s, int c)
{
	const char *p = s + strlen(s);

	do {
		if (*p == (char)c)
			return (char *)p;
	} while (--p >= s);

	return NULL;
}

/**
 * @brief Locate the first occurrence of a substring in a string.
 *
 * @param[in] s1 String to search.
 * @param[in] s2 Substring to find.
 * @return Pointer to the start of the first match, or NULL if @p s2 is not
 *         present.
 */
char *strstr(const char *s1, const char *s2)
{
	register const char *s = s1;
	register const char *p = s2;

	do {
		if (!*p) {
			return (char *)s1;
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

/**
 * @brief Search a memory region for the first occurrence of a byte.
 *
 * @param[in] src Start of the memory region.
 * @param[in] val Byte value to find, interpreted as an unsigned char.
 * @param[in] cnt Number of bytes to scan.
 * @return Pointer to the matching byte, or NULL if it is not found.
 */
void *memchr(const void *src, int val, unsigned int cnt)
{
	const unsigned char *s = src;

	while (cnt) {
		if (*s == (unsigned char)val)
			return (void *)s;
		s++;
		cnt--;
	}

	return NULL;
}

/**
 * @brief Copy at most n characters from one string to another.
 *
 * The destination is padded with null bytes if the source is shorter than
 * @p n.  No null terminator is appended when the source is longer.
 *
 * @param[out] dest Destination buffer.
 * @param[in] src Source string.
 * @param[in] n Maximum number of characters to copy.
 * @return A pointer to @p dest.
 */
char *strncpy(char *dest, const char *src, unsigned int n)
{
	char *tmp = dest;

	while (n) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		n--;
	}
	return dest;
}

/**
 * @brief Copy cnt bytes between overlapping memory regions.
 *
 * Copying is performed forward or backward as required so that the source
 * remains valid for overlapping buffers.
 *
 * @param[out] dst Destination buffer.
 * @param[in] src Source buffer.
 * @param[in] cnt Number of bytes to copy.
 * @return A pointer to @p dst.
 */
void *memmove(void *dst, const void *src, unsigned int cnt)
{
	char *p, *s;

	if (dst <= src) {
		p = (char *)dst;
		s = (char *)src;
		while (cnt--)
			*p++ = *s++;
	} else {
		p = (char *)dst + cnt;
		s = (char *)src + cnt;
		while (cnt--)
			*--p = *--s;
	}

	return dst;
}

#ifdef CONFIG_SPRINTF

/**
 * @brief Format a message into a buffer without a size limit.
 *
 * @param[out] buf Destination buffer.
 * @param[in] fmt printf-style format string.
 * @return Number of characters written, excluding the null terminator.
 */
int sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(buf, ~(size_t)0, fmt, ap);
	va_end(ap);

	return rv;
}

/**
 * @brief Format a message into a buffer with a size limit.
 *
 * @param[out] buf Destination buffer.
 * @param[in] n Capacity of @p buf in bytes.
 * @param[in] fmt printf-style format string.
 * @return Number of characters that would have been written, excluding the
 *         null terminator.
 */
int snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(buf, n, fmt, ap);
	va_end(ap);
	return rv;
}

/** @enum flags
 *  @brief Conversion flags for printf-style integer and float formatting.
 */
enum flags {
	FL_ZERO = 0x01, /* Zero modifier */
	FL_MINUS = 0x02, /* Minus modifier */
	FL_PLUS = 0x04, /* Plus modifier */
	FL_TICK = 0x08, /* ' modifier */
	FL_SPACE = 0x10, /* Space modifier */
	FL_HASH = 0x20, /* # modifier */
	FL_SIGNED = 0x40, /* Number is signed */
	FL_UPPER = 0x80, /* Upper case digits */
};

/*
 * These may have to be adjusted on certain implementations
 */
/** @enum ranks
 *  @brief Relative sizes of the integer conversion types.
 */
enum ranks {
	rank_char = -2,
	rank_short = -1,
	rank_int = 0,
	rank_long = 1,
	rank_longlong = 2,
};

#define MIN_RANK rank_char
#define MAX_RANK rank_longlong
#define INTMAX_RANK rank_longlong
#define SIZE_T_RANK rank_long
#define PTRDIFF_T_RANK rank_long

#define EMIT(x)                     \
	({                          \
		if (o < n) {        \
			*q++ = (x); \
		}                   \
		o++;                \
	})

/**
 * @brief Render an integer value according to the conversion parameters.
 *
 * Emits sign, base prefix, padding, digit grouping, and the digit body of
 * an integer conversion into @p q, honouring the width and precision.
 *
 * @param[out] q Output buffer for the rendered digits.
 * @param[in] n Remaining capacity of @p q.
 * @param[in] val Value to format.
 * @param[in] flags Conversion flags from @ref flags.
 * @param[in] base Radix of the conversion.
 * @param[in] width Minimum field width.
 * @param[in] prec Precision (minimum digit count), or -1 for none.
 * @return Total number of characters the conversion produces.
 */
static size_t format_int(char *q, size_t n, uintmax_t val, enum flags flags, int base, int width, int prec)
{
	char *qq;
	size_t o = 0, oo;
	static const char lcdigits[] = "0123456789abcdef";
	static const char ucdigits[] = "0123456789ABCDEF";
	const char *digits;
	uintmax_t tmpval;
	int minus = 0;
	int ndigits = 0, nchars;
	int tickskip, b4tick;

	/*
	 * Select type of digits
	 */
	digits = (flags & FL_UPPER) ? ucdigits : lcdigits;

	/*
	 * If signed, separate out the minus
	 */
	if ((flags & FL_SIGNED) && ((intmax_t)val < 0)) {
		minus = 1;
		val = (uintmax_t)(-(intmax_t)val);
	}

	/*
	 * Count the number of digits needed.  This returns zero for 0
	 */
	tmpval = val;
	while (tmpval) {
		tmpval /= base;
		ndigits++;
	}

	/*
	 * Adjust ndigits for size of output
	 */
	if ((flags & FL_HASH) && (base == 8)) {
		if (prec < ndigits + 1)
			prec = ndigits + 1;
	}

	if (ndigits < prec) {
		ndigits = prec; /* Mandatory number padding */
	} else if (val == 0) {
		ndigits = 1; /* Zero still requires space */
	}

	/*
	 * For ', figure out what the skip should be
	 */
	if (flags & FL_TICK) {
		tickskip = (base == 16) ? 4 : 3;
	} else {
		tickskip = ndigits; /* No tick marks */
	}

	/*
	 * Tick marks aren't digits, but generated by the number converter
	 */
	ndigits += (ndigits - 1) / tickskip;

	/*
	 * Now compute the number of nondigits
	 */
	nchars = ndigits;

	if (minus || (flags & (FL_PLUS | FL_SPACE)))
		nchars++; /* Need space for sign */
	if ((flags & FL_HASH) && (base == 16)) {
		nchars += 2; /* Add 0x for hex */
		width += 2;
	}

	/*
	 * Emit early space padding
	 */
	if (!(flags & (FL_MINUS | FL_ZERO)) && (width > nchars)) {
		while (width > nchars) {
			EMIT(' ');
			width--;
		}
	}

	/*
	 * Emit nondigits
	 */
	if (minus)
		EMIT('-');
	else if (flags & FL_PLUS)
		EMIT('+');
	else if (flags & FL_SPACE)
		EMIT(' ');

	if ((flags & FL_HASH) && (base == 16)) {
		EMIT('0');
		EMIT((flags & FL_UPPER) ? 'X' : 'x');
	}

	/*
	 * Emit zero padding
	 */
	if (((flags & (FL_MINUS | FL_ZERO)) == FL_ZERO) && (width > ndigits)) {
		while (width > nchars) {
			EMIT('0');
			width--;
		}
	}

	/*
	 * Generate the number.  This is done from right to left
	 */
	q += ndigits; /* Advance the pointer to end of number */
	o += ndigits;
	qq = q;
	oo = o; /* Temporary values */

	b4tick = tickskip;
	while (ndigits > 0) {
		if (!b4tick--) {
			qq--;
			oo--;
			ndigits--;
			if (oo < n)
				*qq = '_';
			b4tick = tickskip - 1;
		}
		qq--;
		oo--;
		ndigits--;
		if (oo < n)
			*qq = digits[val % base];
		val /= base;
	}

	/*
	 * Emit late space padding
	 */
	while ((flags & FL_MINUS) && (width > nchars)) {
		EMIT(' ');
		width--;
	}

	return o;
}

#define CVT_BUFSZ (309 + 43)

/**
 * @brief Split a double into its integral and fractional parts.
 *
 * @param[in] x Value to split.
 * @param[out] iptr Receives the signed integral part of @p x.
 * @return The signed fractional part of @p x.
 */
static double modf(double x, double *iptr)
{
	union {
		double f;
		uint64_t i;
	} u = { x };
	uint64_t mask;
	int e = (int)(u.i >> 52 & 0x7ff) - 0x3ff;

	/* no fractional part */
	if (e >= 52) {
		*iptr = x;
		if (e == 0x400 && u.i << 12 != 0) /* nan */
			return x;
		u.i &= 1ULL << 63;
		return u.f;
	}

	/* no integral part*/
	if (e < 0) {
		u.i &= 1ULL << 63;
		*iptr = u.f;
		return x;
	}

	mask = -1ULL >> 12 >> e;
	if ((u.i & mask) == 0) {
		*iptr = x;
		u.i &= 1ULL << 63;
		return u.f;
	}
	u.i &= ~mask;
	*iptr = u.f;
	return x - u.f;
}

/**
 * @brief Convert a double to a run of ASCII digits with rounding.
 *
 * Generates the significant digits of @p arg together with the decimal
 * point position, optionally using scientific (E) style.  The caller must
 * provide a buffer of at least CVT_BUFSZ bytes.
 *
 * @param[in] arg Value to convert.
 * @param[in] ndigits Number of significant digits to produce.
 * @param[out] decpt Receives the decimal point position.
 * @param[out] sign Receives non-zero when @p arg is negative.
 * @param[out] buf Buffer receiving the null-terminated digit string.
 * @param[in] eflag Non-zero for scientific notation.
 * @return A pointer to @p buf.
 */
static char *cvt(double arg, int ndigits, int *decpt, int *sign, char *buf, int eflag)
{
	int r2;
	double fi, fj;
	char *p, *p1;

	if (ndigits < 0)
		ndigits = 0;
	if (ndigits >= CVT_BUFSZ - 1)
		ndigits = CVT_BUFSZ - 2;

	r2 = 0;
	*sign = 0;
	p = &buf[0];

	if (arg < 0) {
		*sign = 1;
		arg = -arg;
	}
	arg = modf(arg, &fi);
	p1 = &buf[CVT_BUFSZ];

	if (fi != 0) {
		p1 = &buf[CVT_BUFSZ];
		while (fi != 0) {
			fj = modf(fi / 10, &fi);
			*--p1 = (int)((fj + .03) * 10) + '0';
			r2++;
		}
		while (p1 < &buf[CVT_BUFSZ])
			*p++ = *p1++;
	} else if (arg > 0) {
		while ((fj = arg * 10) < 1) {
			arg = fj;
			r2--;
		}
	}

	p1 = &buf[ndigits];
	if (eflag == 0)
		p1 += r2;
	*decpt = r2;
	if (p1 < &buf[0]) {
		buf[0] = '\0';
		return buf;
	}

	while (p <= p1 && p < &buf[CVT_BUFSZ]) {
		arg *= 10;
		arg = modf(arg, &fj);
		*p++ = (int)fj + '0';
	}

	if (p1 >= &buf[CVT_BUFSZ]) {
		buf[CVT_BUFSZ - 1] = '\0';
		return buf;
	}
	p = p1;
	*p1 += 5;

	while (*p1 > '9') {
		*p1 = '0';
		if (p1 > buf)
			++*--p1;
		else {
			*p1 = '1';
			(*decpt)++;
			if (eflag == 0) {
				if (p > buf)
					*p = '0';
				p++;
			}
		}
	}

	*p = '\0';
	return buf;
}

/**
 * @brief Format a double in e, f, or g style into a buffer.
 *
 * @param[in] value Value to format.
 * @param[out] buffer Buffer receiving the null-terminated result.
 * @param[in] fmt Conversion style: 'e', 'E', 'f', 'g', or 'G'.
 * @param[in] precision Number of digits after the decimal point.
 */
static void cfltcvt(double value, char *buffer, char fmt, int precision)
{
	int decpt, sign, exp, pos;
	char *digits = 0;
	char cvtbuf[CVT_BUFSZ];
	int capexp = 0;
	int magnitude;

	if (fmt == 'G' || fmt == 'E') {
		capexp = 1;
		fmt += 'a' - 'A';
	}

	if (fmt == 'g') {
		digits = cvt(value, precision, &decpt, &sign, cvtbuf, 1);

		magnitude = decpt - 1;
		if (magnitude < -4 || magnitude > precision - 1) {
			fmt = 'e';
			precision -= 1;
		} else {
			fmt = 'f';
			precision -= decpt;
		}
	}

	if (fmt == 'e') {
		digits = cvt(value, precision + 1, &decpt, &sign, cvtbuf, 1);

		if (sign)
			*buffer++ = '-';
		*buffer++ = *digits;
		if (precision > 0)
			*buffer++ = '.';
		memcpy(buffer, digits + 1, precision);
		buffer += precision;
		*buffer++ = capexp ? 'E' : 'e';

		if (decpt == 0) {
			if (value == 0.0)
				exp = 0;
			else
				exp = -1;
		} else
			exp = decpt - 1;

		if (exp < 0) {
			*buffer++ = '-';
			exp = -exp;
		} else
			*buffer++ = '+';

		buffer[2] = (exp % 10) + '0';
		exp = exp / 10;
		buffer[1] = (exp % 10) + '0';
		exp = exp / 10;
		buffer[0] = (exp % 10) + '0';
		buffer += 3;
	} else if (fmt == 'f') {
		digits = cvt(value, precision, &decpt, &sign, cvtbuf, 0);

		if (sign)
			*buffer++ = '-';
		if (*digits) {
			if (decpt <= 0) {
				*buffer++ = '0';
				*buffer++ = '.';
				for (pos = 0; pos < -decpt; pos++)
					*buffer++ = '0';
				while (*digits)
					*buffer++ = *digits++;
			} else {
				pos = 0;
				while (*digits) {
					if (pos++ == decpt)
						*buffer++ = '.';
					*buffer++ = *digits++;
				}
			}
		} else {
			*buffer++ = '0';
			if (precision > 0) {
				*buffer++ = '.';
				for (pos = 0; pos < precision; pos++)
					*buffer++ = '0';
			}
		}
	}

	*buffer = '\0';
}

/**
 * @brief Force a decimal point into a digit string.
 *
 * Inserts a '.' before an existing exponent or appends one at the end of
 * the string when neither a point nor an exponent is present.
 *
 * @param[in,out] buffer Null-terminated digit string to modify.
 */
static void forcdecpt(char *buffer)
{
	while (*buffer) {
		if (*buffer == '.')
			return;
		if (*buffer == 'e' || *buffer == 'E')
			break;
		buffer++;
	}

	if (*buffer) {
		int n = strlen(buffer);
		while (n > 0) {
			buffer[n + 1] = buffer[n];
			n--;
		}

		*buffer = '.';
	} else {
		*buffer++ = '.';
		*buffer = '\0';
	}
}

/**
 * @brief Remove trailing fractional zeros from a formatted number.
 *
 * Trims insignificant zeroes after the decimal point, keeping the exponent
 * intact, and removes a now-redundant decimal point.
 *
 * @param[in,out] buffer Null-terminated number string to modify.
 */
static void cropzeros(char *buffer)
{
	char *stop;

	while (*buffer && *buffer != '.')
		buffer++;
	if (*buffer++) {
		while (*buffer && *buffer != 'e' && *buffer != 'E')
			buffer++;
		stop = buffer--;
		while (*buffer == '0')
			buffer--;
		if (*buffer == '.')
			buffer--;
		while ((*++buffer = *stop++))
			;
	}
}

/**
 * @brief Render a floating-point value according to the conversion flags.
 *
 * Formats @p val in the requested style with sign, padding, and precision
 * handling, writing into @p q up to its remaining capacity.
 *
 * @param[out] q Output buffer for the rendered characters.
 * @param[in] n Remaining capacity of @p q.
 * @param[in] val Value to format.
 * @param[in] flags Conversion flags from @ref flags.
 * @param[in] fmt Conversion style: 'e', 'E', 'f', 'g', or 'G'.
 * @param[in] width Minimum field width.
 * @param[in] prec Number of digits after the decimal point.
 * @return Total number of characters the conversion produces.
 */
static size_t format_float(char *q, size_t n, double val, enum flags flags, char fmt, int width, int prec)
{
	size_t o = 0;
	char tmp[CVT_BUFSZ];
	char c, sign;
	int len, i;

	if (flags & FL_MINUS)
		flags &= ~FL_ZERO;

	c = (flags & FL_ZERO) ? '0' : ' ';
	sign = 0;
	if (flags & FL_SIGNED) {
		if (val < 0.0) {
			sign = '-';
			val = -val;
			width--;
		} else if (flags & FL_PLUS) {
			sign = '+';
			width--;
		} else if (flags & FL_SPACE) {
			sign = ' ';
			width--;
		}
	}

	if (prec < 0)
		prec = 6;
	else if (prec == 0 && fmt == 'g')
		prec = 1;

	cfltcvt(val, tmp, fmt, prec);

	if ((flags & FL_HASH) && prec == 0)
		forcdecpt(tmp);

	if (fmt == 'g' && !(flags & FL_HASH))
		cropzeros(tmp);

	len = strlen(tmp);
	width -= len;

	if (!(flags & (FL_ZERO | FL_MINUS)))
		while (width-- > 0)
			EMIT(' ');

	if (sign)
		EMIT(sign);

	if (!(flags & FL_MINUS)) {
		while (width-- > 0)
			EMIT(c);
	}

	for (i = 0; i < len; i++)
		EMIT(tmp[i]);

	while (width-- > 0)
		EMIT(' ');

	return o;
}

/**
 * @brief Format a message using a va_list into a bounded buffer.
 *
 * Supports the standard integer, pointer, character, and string
 * conversions together with the floating-point styles when available.
 *
 * @param[out] buf Destination buffer.
 * @param[in] n Capacity of @p buf in bytes.
 * @param[in] fmt printf-style format string.
 * @param[in] ap Variable argument list.
 * @return Number of characters that would have been written, excluding the
 *         null terminator.
 */
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
	const char *p = fmt;
	char ch;
	char *q = buf;
	size_t o = 0; /* Number of characters output */
	uintmax_t val = 0;
	int rank = rank_int; /* Default rank */
	int width = 0;
	int prec = -1;
	int base;
	size_t sz;
	enum flags flags = 0;
	enum {
		st_normal, /* Ground state */
		st_flags, /* Special flags */
		st_width, /* Field width */
		st_prec, /* Field precision */
		st_modifiers, /* Length or conversion modifiers */
	} state = st_normal;
	const char *sarg; /* %s string argument */
	char carg; /* %c char argument */
	int slen; /* String length */

	while ((ch = *p++)) {
		switch (state) {
		case st_normal:
			if (ch == '%') {
				state = st_flags;
				flags = 0;
				rank = rank_int;
				width = 0;
				prec = -1;
			} else {
				EMIT(ch);
			}
			break;

		case st_flags:
			switch (ch) {
			case '-':
				flags |= FL_MINUS;
				break;
			case '+':
				flags |= FL_PLUS;
				break;
			case '\'':
				flags |= FL_TICK;
				break;
			case ' ':
				flags |= FL_SPACE;
				break;
			case '#':
				flags |= FL_HASH;
				break;
			case '0':
				flags |= FL_ZERO;
				break;
			default:
				state = st_width;
				p--; /* Process this character again */
				break;
			}
			break;

		case st_width:
			if (ch >= '0' && ch <= '9') {
				width = width * 10 + (ch - '0');
			} else if (ch == '*') {
				width = va_arg(ap, int);
				if (width < 0) {
					width = -width;
					flags |= FL_MINUS;
				}
			} else if (ch == '.') {
				prec = 0; /* Precision given */
				state = st_prec;
			} else {
				state = st_modifiers;
				p--; /* Process this character again */
			}
			break;

		case st_prec:
			if (ch >= '0' && ch <= '9') {
				prec = prec * 10 + (ch - '0');
			} else if (ch == '*') {
				prec = va_arg(ap, int);
				if (prec < 0)
					prec = -1;
			} else {
				state = st_modifiers;
				p--; /* Process this character again */
			}
			break;

		case st_modifiers:
			switch (ch) {
			/*
			 * Length modifiers - nonterminal sequences
			 */
			case 'h':
				rank--; /* Shorter rank */
				break;
			case 'l':
				rank++; /* Longer rank */
				break;
			case 'j':
				rank = INTMAX_RANK;
				break;
			case 'z':
				rank = SIZE_T_RANK;
				break;
			case 't':
				rank = PTRDIFF_T_RANK;
				break;
			case 'L':
			case 'q':
				rank += 2;
				break;
			default:
				/*
				 * Next state will be normal
				 */
				state = st_normal;

				/*
				 * Canonicalize rank
				 */
				if (rank < MIN_RANK)
					rank = MIN_RANK;
				else if (rank > MAX_RANK)
					rank = MAX_RANK;

				switch (ch) {
				case 'P': /* Upper case pointer */
					flags |= FL_UPPER;
					break;
				case 'p': /* Pointer */
					base = 16;
					prec = (8 * sizeof(void *) + 3) / 4;
					flags |= FL_HASH;
					val = (uintmax_t)(uintptr_t)va_arg(ap, void *);
					goto is_integer;

				case 'd': /* Signed decimal output */
				case 'i':
					base = 10;
					flags |= FL_SIGNED;
					switch (rank) {
					case rank_char:
						/* Yes, all these casts are needed */
						val = (uintmax_t)(intmax_t)(signed char)va_arg(ap, signed int);
						break;
					case rank_short:
						val = (uintmax_t)(intmax_t)(signed short)va_arg(ap, signed int);
						break;
					case rank_int:
						val = (uintmax_t)(intmax_t)va_arg(ap, signed int);
						break;
					case rank_long:
						val = (uintmax_t)(intmax_t)va_arg(ap, signed long);
						break;
					case rank_longlong:
						val = (uintmax_t)(intmax_t)va_arg(ap, signed long long);
						break;
					}
					goto is_integer;
				case 'o': /* Octal */
					base = 8;
					goto is_unsigned;
				case 'u': /* Unsigned decimal */
					base = 10;
					goto is_unsigned;
				case 'X': /* Upper case hexadecimal */
					flags |= FL_UPPER;
					base = 16;
					goto is_unsigned;
				case 'x': /* Hexadecimal */
					base = 16;
					goto is_unsigned;

is_unsigned:
					switch (rank) {
					case rank_char:
						val = (uintmax_t)(unsigned char)va_arg(ap, unsigned int);
						break;
					case rank_short:
						val = (uintmax_t)(unsigned short)va_arg(ap, unsigned int);
						break;
					case rank_int:
						val = (uintmax_t)va_arg(ap, unsigned int);
						break;
					case rank_long:
						val = (uintmax_t)va_arg(ap, unsigned long);
						break;
					case rank_longlong:
						val = (uintmax_t)va_arg(ap, unsigned long long);
						break;
					}

is_integer:
					sz = format_int(q, (o < n) ? n - o : 0, val, flags, base, width, prec);
					q += sz;
					o += sz;
					break;

				case 'c': /* Character */
					carg = (char)va_arg(ap, int);
					sarg = &carg;
					slen = 1;
					goto is_string;
				case 's': /* String */
					sarg = va_arg(ap, const char *);
					sarg = sarg ? sarg : "(null)";
					slen = strlen(sarg);
					goto is_string;

is_string: {
	char sch;
	int i;

	if (prec != -1 && slen > prec)
		slen = prec;

	if (width > slen && !(flags & FL_MINUS)) {
		char pad = (flags & FL_ZERO) ? '0' : ' ';
		while (width > slen) {
			EMIT(pad);
			width--;
		}
	}
	for (i = slen; i; i--) {
		sch = *sarg++;
		EMIT(sch);
	}
	if (width > slen && (flags & FL_MINUS)) {
		while (width > slen) {
			EMIT(' ');
			width--;
		}
	}
} break;

				case 'n': {
					/*
					 * Output the number of characters written
					 */
					switch (rank) {
					case rank_char:
						*va_arg(ap, signed char *) = o;
						break;
					case rank_short:
						*va_arg(ap, signed short *) = o;
						break;
					case rank_int:
						*va_arg(ap, signed int *) = o;
						break;
					case rank_long:
						*va_arg(ap, signed long *) = o;
						break;
					case rank_longlong:
						*va_arg(ap, signed long long *) = o;
						break;
					}
				} break;

				case 'E':
				case 'G':
				case 'e':
				case 'f':
				case 'g':
					sz = format_float(q, (o < n) ? n - o : 0, (double)(va_arg(ap, double)), flags | FL_SIGNED, ch, width, prec);
					q += sz;
					o += sz;
					break;

				default: /* Anything else, including % */
					EMIT(ch);
					break;
				}
				break;
			}
			break;
		}
	}

	/*
	 * Null-terminate the string
	 */
	if (o < n)
		*q = '\0'; /* No overflow */
	else if (n > 0)
		buf[n - 1] = '\0'; /* Overflow - terminate at end of buffer */

	return o;
}

#endif // CONFIG_SPRINTF
