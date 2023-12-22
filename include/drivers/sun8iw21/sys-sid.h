/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN8IW21_SYS_SID_H__
#define __SUN8IW21_SYS_SID_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#include <reg-ncat.h>

static const struct sid_section_t {
    char *name;
    uint32_t offset;
    uint32_t size_bits;
} sids[] = {
        {"chipid", 0x0000, 128},
        {"brom-conf-try", 0x0010, 32},
        {"thermal-sensor", 0x0014, 64},
        {"ft-zone", 0x001c, 128},
        {"reserved1", 0x002c, 96},
        {"write-protect", 0x0038, 32},
        {"read-protect", 0x003c, 32},
        {"lcjs", 0x0040, 32},
        {"reserved2", 0x0044, 800},
        {"rotpk", 0x00a8, 256},
        {"reserved3", 0x00c8, 448},
};

enum {
    SID_PRCTL = SUNXI_SID_BASE + 0x040,
    SID_PRKEY = SUNXI_SID_BASE + 0x050,
    SID_RDKEY = SUNXI_SID_BASE + 0x060,
    EFUSE_HV_SWITCH = SUNXI_RTC_BASE + 0x204,
};

uint32_t efuse_read(uint32_t offset);

void efuse_write(uint32_t offset, uint32_t value);

#endif// __SUN8IW21_SYS_SID_H__