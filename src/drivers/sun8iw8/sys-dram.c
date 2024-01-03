/* SPDX-License-Identifier: Apache-2.0 */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <sys-dram.h>
#include <sys-rtc.h>

uint64_t sunxi_dram_init(dram_para_t *para) {
    return 0;
}