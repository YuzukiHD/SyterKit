/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN55IW3_SYS_SID_H__
#define __SUN55IW3_SYS_SID_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#include <reg-ncat.h>

static const struct sid_section_t {
	char * name;
	uint32_t offset;
	uint32_t size_bits;
} sids[] = {
    {"chipid", 0x00, 128},
    {"brom-config", 0x10, 32},
    {"aldo-fix", 0x14, 1},
    {"thermal-sensor", 0x30, 64},
    {"tf-zone", 0x28, 128},
    {"oem-program", 0x3C, 160},
    {"write-protect", 0x80, 32},
    {"read-protect", 0x84, 32},
    {"lcjs", 0x88, 32},
    {"attr", 0x90, 32},
    {"huk", 0x94, 192},
    {"reserved1", 0xAC, 64},
    {"rotpk", 0xB4, 256},    
    {"ssk", 0xD4, 128},             
    {"rssk", 0xF4, 256},           
    {"sn", 0xB0, 192},
    {"nv1", 0x124, 32},
    {"nv2", 0x128, 32},
    {"hdcp-hash", 0x114, 128},
    {"backup-key", 0x164, 192},
    {"backup-key2", 0x1A4, 72}
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

#endif// __SUN55IW3_SYS_SID_H__