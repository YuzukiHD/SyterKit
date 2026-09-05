/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file memcmp.c
 * @brief Freestanding byte-wise memory comparison for RISC-V firmware.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

/**
 * @brief Compare two memory regions lexicographically.
 * @param[in] s1 First byte region.
 * @param[in] s2 Second byte region.
 * @param[in] n Number of bytes to compare.
 * @return Negative, zero, or positive according to the first differing byte.
 */
int memcmp(const void *s1, const void *s2, unsigned int n)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, n--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
