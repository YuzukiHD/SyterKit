/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <log.h>

long qemu_sys_write(long fd, const void *buffer, unsigned long size);
void qemu_sys_exit(long status) __attribute__((noreturn));

static void qemu_putchar(char value) {
	(void) qemu_sys_write(1, &value, 1);
}

void qemu_puts(const char *text) {
	while (*text) {
		if (*text == '\n')
			qemu_putchar('\r');
		qemu_putchar(*text++);
	}
}

static void qemu_print_unsigned(uintptr_t value, unsigned int radix,
				unsigned int width, char padding) {
	char buffer[2U * sizeof(value) + 1U];
	unsigned int index = sizeof(buffer);

	do {
		unsigned int digit = (unsigned int) (value % radix);

		buffer[--index] = (char) (digit < 10U ? '0' + digit : 'a' + digit - 10U);
		value /= radix;
	} while (value);
	while (sizeof(buffer) - index < width)
		buffer[--index] = padding;
	while (index < sizeof(buffer))
		qemu_putchar(buffer[index++]);
}

static void qemu_vprintf(const char *format, va_list args) {
	while (*format) {
		unsigned int width = 0;
		unsigned int long_argument = 0;
		char padding = ' ';
		char specifier;

		if (*format != '%') {
			qemu_putchar(*format++);
			continue;
		}
		format++;
		if (*format == '0') {
			padding = '0';
			format++;
		}
		while (*format >= '0' && *format <= '9')
			width = width * 10U + (unsigned int) (*format++ - '0');
		if (*format == 'l') {
			long_argument = 1U;
			format++;
		}
		specifier = *format++;
		switch (specifier) {
		case '%':
			qemu_putchar('%');
			break;
		case 'c':
			qemu_putchar((char) va_arg(args, int));
			break;
		case 'd': {
			int value = va_arg(args, int);

			if (value < 0) {
				qemu_putchar('-');
				qemu_print_unsigned((uintptr_t) -value, 10U, width, padding);
			} else {
				qemu_print_unsigned((uintptr_t) value, 10U, width, padding);
			}
			break;
		}
		case 'p':
			qemu_puts("0x");
			qemu_print_unsigned((uintptr_t) va_arg(args, void *), 16U,
					    2U * sizeof(uintptr_t), '0');
			break;
		case 's':
			qemu_puts(va_arg(args, const char *));
			break;
		case 'u':
			qemu_print_unsigned(long_argument ?
					    (uintptr_t) va_arg(args, unsigned long) :
					    (uintptr_t) va_arg(args, unsigned int),
					    10U, width,
					    padding);
			break;
		case 'x':
		case 'X':
			qemu_print_unsigned(long_argument ?
					    (uintptr_t) va_arg(args, unsigned long) :
					    (uintptr_t) va_arg(args, unsigned int),
					    16U, width,
					    padding);
			break;
		default:
			qemu_putchar('%');
			qemu_putchar(specifier);
			break;
		}
	}
}

void printk(int level, const char *format, ...) {
	va_list args;

	(void) level;
	va_start(args, format);
	qemu_vprintf(format, args);
	va_end(args);
}

void qemu_exit(int success) {
	qemu_sys_exit(success ? 0 : 1);
}
