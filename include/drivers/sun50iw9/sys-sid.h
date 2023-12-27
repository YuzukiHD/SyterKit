/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN50IW9_SYS_SID_H__
#define __SUN50IW9_SYS_SID_H__

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
        {"brom-conf", 0x0010, 32},
        {"thermal-sensor", 0x0014, 64},
        {"ft-zone", 0x001c, 128},
        {"oem", 0x002c, 160},
        {"write-protect", 0x0040, 32},
        {"read-protect", 0x0044, 32},
        {"jtag-lcjs", 0x0048, 32},
        {"jtag-attr", 0x004C, 32},
        {"efuse-huk", 0x0050, 192},
        {"efuse-ind", 0x0068, 32},
        {"efuse-id", 0x006c, 32},
        {"rotpk", 0x0070, 256},
        {"ssk", 0x0090, 128},
        {"rssk", 0x00a0, 256},
        {"sn", 0x00b0, 192},
        {"nv1", 0x00c8, 32},
        {"nv2", 0x00cc, 32},
        {"hdcp-hash", 0x00d0, 128},
        {"backup-key0", 0x00e0, 192},
        {"backup-key1", 0x00f8, 72},
};

enum {
    SID_PRCTL = SUNXI_SID_BASE + 0x040,
    SID_PRKEY = SUNXI_SID_BASE + 0x050,
    SID_RDKEY = SUNXI_SID_BASE + 0x060,
    SJTAG_AT0 = SUNXI_SID_BASE + 0x080,
    SJTAG_AT1 = SUNXI_SID_BASE + 0x084,
    SJTAG_S = SUNXI_SID_BASE + 0x088,
    SID_EFUSE = SUNXI_SID_BASE + 0x200,
    SID_SECURE_MODE = SUNXI_SID_BASE + 0xA0,
    EFUSE_HV_SWITCH = SUNXI_RTC_BASE + 0x204,
};

uint32_t syter_efuse_read(uint32_t offset);

void syter_efuse_write(uint32_t offset, uint32_t value);

void syter_efuse_dump(void);

#endif// __SUN50IW9_SYS_SID_H__