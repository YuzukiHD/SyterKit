/* SPDX-License-Identifier: GPL-2.0+ */

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "syter_test.h"

static int failures;

static size_t local_strlen(const char *text)
{
	size_t length = 0;

	while (text && text[length])
		length++;
	return length;
}

static int local_strcmp(const char *left, const char *right)
{
	while (*left && *left == *right) {
		left++;
		right++;
	}
	return (unsigned char)*left - (unsigned char)*right;
}

static void write_text(const char *text)
{
	size_t length = local_strlen(text);

	while (length) {
		ssize_t written = write(STDOUT_FILENO, text, length);

		if (written <= 0)
			return;
		text += written;
		length -= (size_t)written;
	}
}

static void write_number(unsigned long long value)
{
	char buffer[24];
	size_t index = sizeof(buffer);

	buffer[--index] = '\0';
	do {
		buffer[--index] = (char)('0' + value % 10U);
		value /= 10U;
	} while (value);
	write_text(&buffer[index]);
}

void test_fail(const char *expression, const char *file, int line)
{
	failures++;
	write_text("ASSERT ");
	write_text(file);
	write_text(":");
	write_number((unsigned int)line);
	write_text(" ");
	write_text(expression);
	write_text("\n");
}

void test_fail_value(const char *expression, unsigned long long expected, unsigned long long actual, const char *file, int line)
{
	test_fail(expression, file, line);
	write_text("  expected=");
	write_number(expected);
	write_text(" actual=");
	write_number(actual);
	write_text("\n");
}

void test_expect_string(const char *expression, const char *expected, const char *actual, const char *file, int line)
{
	if (!expected || !actual || local_strcmp(expected, actual)) {
		test_fail(expression, file, line);
		write_text("  expected=\"");
		write_text(expected ? expected : "(null)");
		write_text("\" actual=\"");
		write_text(actual ? actual : "(null)");
		write_text("\"\n");
	}
}

int test_load_data(const char *case_dir, const char *relative_path, char *buffer, size_t capacity)
{
	char path[1024];
	size_t case_length = local_strlen(case_dir);
	size_t relative_length = local_strlen(relative_path);
	ssize_t count;
	int fd;

	if (!capacity || case_length + relative_length + 2U > sizeof(path))
		return -1;

	for (size_t i = 0; i < case_length; i++)
		path[i] = case_dir[i];
	path[case_length] = '/';
	for (size_t i = 0; i <= relative_length; i++)
		path[case_length + 1U + i] = relative_path[i];

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	count = read(fd, buffer, capacity - 1U);
	close(fd);
	if (count < 0)
		return -1;
	buffer[count] = '\0';
	return (int)count;
}

int test_failure_count(void)
{
	return failures;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		write_text("TEST FAIL " TEST_CASE_NAME "\n");
		return 2;
	}

	test_case_main(argv[1]);
	if (failures) {
		write_text("TEST FAIL " TEST_CASE_NAME "\n");
		return 1;
	}

	write_text("TEST PASS " TEST_CASE_NAME "\n");
	return 0;
}
