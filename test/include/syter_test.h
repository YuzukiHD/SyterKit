/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTER_TEST_H__
#define __SYTER_TEST_H__

#include <stddef.h>

/** @brief Maximum size of a test data file. */
#define TEST_DATA_MAX 4096U

/**
 * @brief Record a failed boolean assertion.
 *
 * @param expression Text of the failed expression.
 * @param file Source file containing the assertion.
 * @param line Source line containing the assertion.
 */
void test_fail(const char *expression, const char *file, int line);

/**
 * @brief Record a failed integer comparison.
 *
 * @param expression Text of the failed expression.
 * @param expected Expected value.
 * @param actual Actual value.
 * @param file Source file containing the assertion.
 * @param line Source line containing the assertion.
 */
void test_fail_value(const char *expression, unsigned long long expected,
		     unsigned long long actual, const char *file, int line);

/**
 * @brief Compare two strings and record a failure if they differ.
 *
 * @param expression Text of the failed expression.
 * @param expected Expected string.
 * @param actual Actual string.
 * @param file Source file containing the assertion.
 * @param line Source line containing the assertion.
 */
void test_expect_string(const char *expression, const char *expected,
			const char *actual, const char *file, int line);

/**
 * @brief Load a data file relative to a test case directory.
 *
 * @param case_dir Test case directory.
 * @param relative_path Path below the test case directory.
 * @param buffer Destination buffer.
 * @param capacity Destination capacity in bytes.
 * @return Number of bytes read, or -1 on failure.
 */
int test_load_data(const char *case_dir, const char *relative_path,
		   char *buffer, size_t capacity);

/**
 * @brief Return the number of failed assertions.
 *
 * @return Failed assertion count.
 */
int test_failure_count(void);

/**
 * @brief Test implementation supplied by each case.
 *
 * @param case_dir Test case directory.
 */
void test_case_main(const char *case_dir);

#define TEST_ASSERT(expression) \
	do { \
		if (!(expression)) \
			test_fail(#expression, __FILE__, __LINE__); \
	} while (0)

#define TEST_EQ(expected, actual) \
	do { \
		unsigned long long test_expected_ = (unsigned long long) (expected); \
		unsigned long long test_actual_ = (unsigned long long) (actual); \
		if (test_expected_ != test_actual_) \
			test_fail_value(#actual, test_expected_, test_actual_, \
					__FILE__, __LINE__); \
	} while (0)

#define TEST_STREQ(expected, actual) \
	test_expect_string(#actual, (expected), (actual), __FILE__, __LINE__)

#endif
