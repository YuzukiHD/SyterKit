/* SPDX-License-Identifier: GPL-2.0+ */
/** @file format.c
 *  @brief Small callback-based formatter used by firmware logging.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "format.h"

struct format_state {
	format_putc_t putc;
	void *arg;
	unsigned count;
};

static void emit(struct format_state *s, char c)
{
	s->putc(s->arg, c);
	s->count++;
}

static void emit_repeat(struct format_state *s, char c, int count)
{
	while (count-- > 0)
		emit(s, c);
}

static int string_length(const char *str, int precision)
{
	int length = 0;

	if (!str)
		str = "(null)";
	while (str[length] && (precision < 0 || length < precision))
		length++;
	return length;
}

static int number_digits(unsigned long long value, unsigned base, char *buffer, bool upper)
{
	static const char lower[] = "0123456789abcdef";
	static const char upper_digits[] = "0123456789ABCDEF";
	const char *digits = upper ? upper_digits : lower;
	int length = 0;

	do {
		buffer[length++] = digits[value % base];
		value /= base;
	} while (value);
	return length;
}

unsigned vformat(format_putc_t putc, void *arg, const char *fmt, va_list args)
{
	struct format_state state = { putc, arg, 0 };

	while (*fmt) {
		bool left = false, zero = false, plus = false, space = false;
		bool alternate = false, negative = false;
		int width = 0, precision = -1, length = 0;
		char number[sizeof(unsigned long long) * 8 / 3 + 3];
		const char *str;
		unsigned long long value = 0;
		int digits, padding, precision_zeros, prefix, sign_prefix, base_prefix;

		if (*fmt != '%') {
			emit(&state, *fmt++);
			continue;
		}
		fmt++;
		for (;;) {
			switch (*fmt) {
			case '-':
				left = true;
				fmt++;
				continue;
			case '0':
				zero = true;
				fmt++;
				continue;
			case '+':
				plus = true;
				fmt++;
				continue;
			case ' ':
				space = true;
				fmt++;
				continue;
			case '#':
				alternate = true;
				fmt++;
				continue;
			default:
				break;
			}
			break;
		}
		if (*fmt == '*') {
			width = va_arg(args, int);
			if (width < 0) {
				left = true;
				width = -width;
			}
			fmt++;
		} else {
			while (*fmt >= '0' && *fmt <= '9')
				width = width * 10 + (*fmt++ - '0');
		}
		if (*fmt == '.') {
			fmt++;
			precision = 0;
			if (*fmt == '*') {
				precision = va_arg(args, int);
				if (precision < 0)
					precision = -1;
				fmt++;
			} else {
				while (*fmt >= '0' && *fmt <= '9')
					precision = precision * 10 + (*fmt++ - '0');
			}
		}
		if (*fmt == 'l') {
			length = 1;
			fmt++;
			if (*fmt == 'l') {
				length = 2;
				fmt++;
			}
		} else if (*fmt == 'z') {
			length = 3;
			fmt++;
		} else if (*fmt == 't') {
			length = 4;
			fmt++;
		} else if (*fmt == 'j') {
			length = 2;
			fmt++;
		}

		{
			char spec = *fmt++;
			switch (spec) {
			case '%':
				emit(&state, '%');
				continue;
			case 'c':
				value = (unsigned char)va_arg(args, int);
				padding = width > 1 ? width - 1 : 0;
				if (!left)
					emit_repeat(&state, ' ', padding);
				emit(&state, (char)value);
				if (left)
					emit_repeat(&state, ' ', padding);
				continue;
			case 's':
				str = va_arg(args, const char *);
				if (!str)
					str = "(null)";
				digits = string_length(str, precision);
				padding = width > digits ? width - digits : 0;
				if (!left)
					emit_repeat(&state, ' ', padding);
				for (int i = 0; i < digits; i++)
					emit(&state, str[i]);
				if (left)
					emit_repeat(&state, ' ', padding);
				continue;
			case 'p':
				value = (uintptr_t)va_arg(args, void *);
				alternate = true;
				/* fall through */
			case 'd':
			case 'i':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				if (spec == 'd' || spec == 'i') {
					long long signed_value;
					if (length == 1)
						signed_value = va_arg(args, long);
					else if (length == 2)
						signed_value = va_arg(args, long long);
					else if (length == 3)
						signed_value = (long long)va_arg(args, size_t);
					else
						signed_value = va_arg(args, int);
					if (signed_value < 0) {
						negative = true;
						value = (unsigned long long)(-(signed_value + 1)) + 1;
					} else
						value = (unsigned long long)signed_value;
				} else if (spec != 'p') {
					if (length == 1)
						value = va_arg(args, unsigned long);
					else if (length == 2)
						value = va_arg(args, unsigned long long);
					else if (length == 3)
						value = va_arg(args, size_t);
					else
						value = va_arg(args, unsigned int);
				}
				{
					unsigned base = spec == 'o' ? 8 : ((spec == 'x' || spec == 'X' || spec == 'p') ? 16 : 10);
					bool upper = spec == 'X';
					digits = number_digits(value, base, number, upper);
					if (precision == 0 && value == 0)
						digits = 0;
					precision_zeros = precision > digits ? precision - digits : 0;
					padding = precision_zeros;
					base_prefix = 0;
					if (alternate && base == 16 && (value || spec == 'p'))
						base_prefix = 2;
					else if (alternate && base == 8 && (value || precision == 0))
						base_prefix = 1;
					sign_prefix = negative || plus || space;
					prefix = sign_prefix + base_prefix;
					if (zero && !left && precision < 0 && width > prefix + digits)
						padding = width - prefix - digits;
					else if (width > prefix + digits + padding)
						padding += width - prefix - digits - padding;
					if (!left && !(zero && precision < 0))
						emit_repeat(&state, ' ', padding - precision_zeros);
					if (negative)
						emit(&state, '-');
					else if (plus)
						emit(&state, '+');
					else if (space)
						emit(&state, ' ');
					if (base_prefix == 2) {
						emit(&state, '0');
						emit(&state, upper ? 'X' : 'x');
					} else if (base_prefix == 1)
						emit(&state, '0');
					if (zero && !left && precision < 0)
						emit_repeat(&state, '0', padding);
					else
						emit_repeat(&state, '0', precision_zeros);
					for (int i = digits - 1; i >= 0; i--)
						emit(&state, number[i]);
					if (left)
						emit_repeat(&state, ' ', padding - precision_zeros);
				}
				continue;
			default:
				emit(&state, '%');
				emit(&state, *(fmt - 1));
				continue;
			}
		}
	}
	return state.count;
}

unsigned format(format_putc_t putc, void *arg, const char *fmt, ...)
{
	va_list args;
	unsigned count;

	va_start(args, fmt);
	count = vformat(putc, arg, fmt, args);
	va_end(args);
	return count;
}
