/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <malloc.h>

#include "syter_test.h"

static unsigned int parse_value(char **cursor)
{
	unsigned int value = 0;

	while (**cursor == ' ' || **cursor == '\n')
		(*cursor)++;
	while (**cursor >= '0' && **cursor <= '9')
		value = value * 10U + (unsigned int)(*(*cursor)++ - '0');
	return value;
}

void test_case_main(const char *case_dir)
{
	static unsigned char heap[2048];
	static unsigned char second_heap[2048];
	char data[TEST_DATA_MAX];
	char *cursor;
	unsigned char *first;
	unsigned char *second;
	unsigned char *resized;
	unsigned char *second_region;
	unsigned int first_size;
	unsigned int second_size;
	unsigned int resize_size;
	int length;

	length = test_load_data(case_dir, "data/sizes.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;
	cursor = data;
	first_size = parse_value(&cursor);
	second_size = parse_value(&cursor);
	resize_size = parse_value(&cursor);

	TEST_EQ(0, malloc_init((uintptr_t)heap, sizeof(heap)));
	first = malloc(first_size);
	second = malloc(second_size);
	TEST_ASSERT(first != NULL && second != NULL);
	TEST_EQ(0, (uintptr_t)first & 15U);
	for (unsigned int index = 0; index < first_size; index++)
		first[index] = (unsigned char)(index + 1U);

	resized = realloc(first, resize_size);
	TEST_ASSERT(resized != NULL);
	for (unsigned int index = 0; index < first_size; index++)
		TEST_EQ((unsigned char)(index + 1U), resized[index]);
	free(second);
	free(resized);
	TEST_ASSERT(malloc(second_size) != NULL);
	TEST_EQ(0, malloc_add_region((uintptr_t)second_heap, sizeof(second_heap)));
	second_region = malloc(second_size);
	TEST_ASSERT((uintptr_t)second_region >= (uintptr_t)second_heap &&
		(uintptr_t)second_region < (uintptr_t)second_heap + sizeof(second_heap));
	free(second_region);
}
