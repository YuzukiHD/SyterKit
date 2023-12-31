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

#define INIT_DRAM_BIN_BASE 0x48000

#define SUNXI_RTC_BASE (0x07000000)
#define SUNXI_RTC_DATA_BASE (SUNXI_RTC_BASE + 0x100)

#define RTC_FEL_INDEX 2

extern uint8_t __ddr_bin_start[];
extern uint8_t __ddr_bin_end[];

uint64_t sunxi_dram_init() {
    uint8_t *src = __ddr_bin_start;
    uint8_t *dst = (uint8_t *) INIT_DRAM_BIN_BASE;

    printk(LOG_LEVEL_DEBUG, "DRAM: load dram init from 0x%08x -> 0x%08x size: %08x\n", src, dst, __ddr_bin_end - __ddr_bin_start);
    memcpy(dst, src, __ddr_bin_end - __ddr_bin_start);

    /* Set RTC data to current time_ms(), Save in RTC_FEL_INDEX */
    rtc_set_start_time_ms();

    printk(LOG_LEVEL_DEBUG, "DRAM: Now jump to 0x%08x run DRAMINIT\n", dst);

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
    ((void (*)(void)) ((void *) INIT_DRAM_BIN_BASE))();

    uint32_t dram_size = rtc_read_data(RTC_FEL_INDEX);

    /* And Restore RTC Flag */
    rtc_clear_fel_flag();

    return dram_size;
}