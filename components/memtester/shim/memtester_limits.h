/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Force-included in the sources of this component. limits.h of the architecture defines
 * LONG_MAX as a 64 bit value even for rv32, which makes sizes.h pick its 64 bit patterns for
 * a 32 bit unsigned long. The values below are the ones of the compiler.
 */
#ifndef MEMTESTER_SHIM_LIMITS_H
#define MEMTESTER_SHIM_LIMITS_H

#include <limits.h>

#undef LONG_MAX
#undef LONG_MIN
#undef ULONG_MAX
#define LONG_MAX  __LONG_MAX__
#define LONG_MIN  (-LONG_MAX - 1L)
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)

#endif
