/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cli/cli.h>

#include "syter_test.h"

static int invocation_count;

static int sample_command(int argc, const char **argv)
{
	invocation_count++;
	return argc + (argv && argv[0] ? 10 : 0);
}

static const msh_command_entry commands[] = {
	{ "sample", sample_command, "sample command", "Usage: sample [arg]\n" },
	msh_command_end,
};

const msh_command_entry *msh_user_commands = commands;

int uart_putchar(int value)
{
	return value;
}

int uart_puts(const char *text)
{
	int count = 0;

	while (text[count])
		count++;
	return count;
}

void printk(int level, const char *format, ...)
{
	(void)level;
	(void)format;
}

void dump_hex(uintptr_t start_addr, uint32_t count)
{
	(void)start_addr;
	(void)count;
}

uint32_t test_mmio_read32(uintptr_t address)
{
	return (uint32_t)address;
}

void test_mmio_write32(uintptr_t address, uint32_t value)
{
	(void)address;
	(void)value;
}

int get_history_count(void)
{
	return 0;
}

const char *history_get(int index)
{
	(void)index;
	return "";
}

void test_case_main(const char *case_dir)
{
	char data[TEST_DATA_MAX];
	const char *arguments[] = { "sample", "argument" };
	int length;

	length = test_load_data(case_dir, "data/usage.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;
	TEST_EQ(12, msh_do_command(commands, 2, arguments));
	TEST_EQ(1, invocation_count);
	TEST_EQ((unsigned long long)-1, msh_do_command(commands, 1, &arguments[1]));
	TEST_STREQ(data, msh_get_command_usage(commands, "sample"));
	TEST_ASSERT(msh_get_command_usage(commands, "missing") == NULL);
}
