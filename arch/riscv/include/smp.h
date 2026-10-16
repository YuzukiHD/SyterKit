/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file smp.h
 * @brief Start secondary Xuantie C907 harts.
 */
#ifndef __SMP_H__
#define __SMP_H__

#define SMP_MAX_HARTS  2
#define SMP_STACK_SIZE 4096

#ifndef __ASSEMBLER__

#include <stdint.h>

/** @brief Function a secondary hart runs; it must not return. */
typedef void (*smp_entry_t)(unsigned long arg);

/**
 * @brief Run @p entry(@p arg) on hart @p hartid.
 *
 * The hart starts at _start, takes the secondary path there, switches on its
 * caches and calls @p entry. Hart 0 is the caller and cannot be started.
 *
 * @return 0 on success, negative when the id is invalid, the SoC has no
 * hook, or the hart did not report in within @p timeout_ms.
 */
int smp_hart_start(unsigned int hartid, smp_entry_t entry, unsigned long arg, uint32_t timeout_ms);

/**
 * @brief SoC hook: power on @p hartid so that it fetches from @p entry_addr.
 *
 * Weak default returns -1.
 */
int smp_soc_hart_start(unsigned int hartid, unsigned long entry_addr);

#endif /* __ASSEMBLER__ */
#endif /* __SMP_H__ */
