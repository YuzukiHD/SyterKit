/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <reg-ncat.h>

#define EFEX_FLAG (0x5AA5A55A)
#define RTC_FEL_INDEX 2
#define RTC_BOOT_INDEX 6

void rtc_write_data(int index, uint32_t val) {
    writel(val, SUNXI_RTC_DATA_BASE + index * 4);
}

uint32_t rtc_read_data(int index) {
    return readl(SUNXI_RTC_DATA_BASE + index * 4);
}

void rtc_set_fel_flag(void) {
    do {
        rtc_write_data(RTC_FEL_INDEX, EFEX_FLAG);
        data_sync_barrier();
    } while (rtc_read_data(RTC_FEL_INDEX) != EFEX_FLAG);
}

uint32_t rtc_probe_fel_flag(void) {
    return rtc_read_data(RTC_FEL_INDEX) == EFEX_FLAG ? 1 : 0;
}

void rtc_clear_fel_flag(void) {
    do {
        rtc_write_data(RTC_FEL_INDEX, 0);
        data_sync_barrier();
    } while (rtc_read_data(RTC_FEL_INDEX) != 0);
}

int rtc_set_bootmode_flag(uint8_t flag) {
    do {
        rtc_write_data(RTC_BOOT_INDEX, flag);
        data_sync_barrier();
    } while (rtc_read_data(RTC_BOOT_INDEX) != flag);

    return 0;
}

int rtc_get_bootmode_flag(void) {
    uint32_t boot_flag;

    /* operation should be same with kernel write rtc */
    boot_flag = rtc_read_data(RTC_BOOT_INDEX);

    return boot_flag;
}
