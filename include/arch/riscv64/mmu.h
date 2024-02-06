/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __MMU_H__
#define __MMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "timer.h"


static inline void data_sync_barrier(void) {
    asm volatile("fence.i");
}

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
