/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <format.h>

void qemu_puts(const char *text);
void qemu_exit(int success);

static char output[256];
static size_t output_length;

static void putc_buffer(void *arg, char value)
{
	(void)arg;
	if (output_length + 1U < sizeof(output))
		output[output_length++] = value;
	output[output_length] = '\0';
}

static size_t string_length(const char *text)
{
	size_t length = 0;
	while (text[length])
		length++;
	return length;
}

static int string_equal(const char *left, const char *right)
{
	while (*left && *left == *right) {
		left++;
		right++;
	}
	return *left == *right;
}

static int run_case(const char *name, const char *expected,
			const char *fmt, ...)
{
	va_list args;
	unsigned count;

	output_length = 0;
	output[0] = '\0';
	va_start(args, fmt);
	count = vformat(putc_buffer, NULL, fmt, args);
	va_end(args);
	if (!string_equal(output, expected) || count != string_length(expected)) {
		qemu_puts("CHECK FAIL ");
		qemu_puts(name);
		qemu_puts("\n");
		return 0;
	}
	qemu_puts("CHECK ");
	qemu_puts(name);
	qemu_puts(" PASS\n");
	return 1;
}

static int run_pointer_case(void)
{
	output_length = 0;
	output[0] = '\0';
	format(putc_buffer, NULL, "%p", (void *)(uintptr_t)0x1234U);
	if (output_length < 6U || output[0] != '0' || output[1] != 'x' ||
	    output[output_length - 4U] != '1' || output[output_length - 1U] != '4') {
		qemu_puts("CHECK FAIL pointer\n");
		return 0;
	}
	qemu_puts("CHECK pointer PASS\n");
	return 1;
}

void test_boot(void)
{
	int passed = 1;

	qemu_puts("TEST START "
#ifdef FORMAT_RISCV64_TEST
		"format_riscv64_qemu"
#else
		"format_arm_qemu"
#endif
		"\n");
	passed &= run_case("sign-width", "-0007", "%+05d", -7);
	passed &= run_case("alternate-hex", "0x00002a", "%#08x", 42U);
	passed &= run_case("string-precision", "abcdef|abc", "%.6s|%.3s", "abcdef", "abcdef");
	passed &= run_case("left-char-percent", "A       |Z|%", "%-8s|%c|%%", "A", 'Z');
	passed &= run_case("precision-sign", "  +00042", "%+8.5d", 42);
	passed &= run_case("octal", "052", "%#o", 42U);
	passed &= run_case("null-string", "(null)", "%s", (char *)0);
	passed &= run_case("long-long", "1122334455667788", "%llx", 0x1122334455667788ULL);
	passed &= run_case("long-long-unsigned", "4294967296", "%llu", 0x100000000ULL);
	passed &= run_case("long-long-signed", "-4294967296", "%lld", -0x100000000LL);
	passed &= run_case("size-value", "123456", "%zu", (size_t)123456U);
	passed &= run_pointer_case();
	if (passed) {
		qemu_puts("TEST PASS "
#ifdef FORMAT_RISCV64_TEST
			"format_riscv64_qemu"
#else
			"format_arm_qemu"
#endif
			"\n");
		qemu_exit(1);
	}
	qemu_puts("TEST FAIL format_arm_qemu\n");
	qemu_exit(0);
}
