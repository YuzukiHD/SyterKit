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

#define INIT_DRAM_BIN_BASE 0x48000

extern uint8_t __ddr_bin_start[];
extern uint8_t __ddr_bin_end[];

uint64_t sunxi_dram_init(void *para) {
    uint8_t *src = __ddr_bin_start;
    uint8_t *dst = (uint8_t *) INIT_DRAM_BIN_BASE;

    printk(LOG_LEVEL_DEBUG, "0x%x -> 0x%x size: %x\n", src, dst, __ddr_bin_start - __ddr_bin_end);
    memcpy(dst, src, __ddr_bin_end - __ddr_bin_start);

    __asm__ __volatile__("isb sy"
                         :
                         :
                         : "memory");
    __asm__ __volatile__("dsb sy"
                         :
                         :
                         : "memory");
    __asm__ __volatile__("dmb sy"
                         :
                         :
                         : "memory");
    syterkit_jmp(INIT_DRAM_BIN_BASE);

    printk(LOG_LEVEL_ERROR, "Dram initsssssss\n\n\n");
}