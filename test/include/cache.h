/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTER_TEST_CACHE_H__
#define __SYTER_TEST_CACHE_H__

void test_data_sync_barrier(void);

static inline void data_sync_barrier(void)
{
	test_data_sync_barrier();
}

#endif
