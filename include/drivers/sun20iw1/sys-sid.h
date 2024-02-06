/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN20IW1_SYS_SID_H__
#define __SUN20IW1_SYS_SID_H__

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
        {"tvout", 0x002c, 32},
        {"tvout-gamma", 0x0030, 64},
        {"oem-program", 0x0038, 64},
        {"write-protect", 0x0040, 32},
        {"read-protect", 0x0044, 32},
        {"reserved1", 0x0048, 64},
        {"huk", 0x0050, 192},
        {"reserved2", 0x0068, 64},
        {"rotpk", 0x0070, 256},
        {"ssk", 0x0090, 256},
        {"rssk", 0x00b0, 128},
        {"hdcp-hash", 0x00c0, 128},
        {"nv1", 0x00d0, 32},
        {"nv2", 0x00d4, 32},
        {"reserved3", 0x00d8, 96},
        {"oem-program-secure", 0x00e4, 224},
};

enum {
    SID_PRCTL = SUNXI_SID_BASE + 0x040,
    SID_PRKEY = SUNXI_SID_BASE + 0x050,
    SID_RDKEY = SUNXI_SID_BASE + 0x060,
    EFUSE_HV_SWITCH = SUNXI_RTC_BASE + 0x204,
};

uint32_t syter_efuse_read(uint32_t offset);

void syter_efuse_write(uint32_t offset, uint32_t value);

void syter_efuse_dump(void);

#endif// __SUN20IW1_SYS_SID_H__