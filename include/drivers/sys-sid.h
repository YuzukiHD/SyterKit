/* SPDX-License-Identifier: Apache-2.0 */

#ifndef _SYS_SID_H_
#define _SYS_SID_H_


#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "log.h"

static const struct sid_section_t {
	char * name;
	uint32_t offset;
	uint32_t size_bits;
} sids[] = {
	{ "chipid",         0x0000,  128 },
	{ "brom-conf-try",  0x0010,   32 },
	{ "thermal-sensor", 0x0014,   64 },
	{ "ft-zone",        0x001c,  128 },
	{ "reserved1",      0x002c,   96 },
	{ "write-protect",  0x0038,   32 },
	{ "read-protect",   0x003c,   32 },
	{ "lcjs",           0x0040,   32 },
	{ "reserved2",      0x0044,  800 },
	{ "rotpk",          0x00a8,  256 },
	{ "reserved3",      0x00c8,  448 },
};

enum {
	SID_PRCTL		= 0x03006000 + 0x040,
	SID_PRKEY		= 0x03006000 + 0x050,
	SID_RDKEY 		= 0x03006000 + 0x060,
	EFUSE_HV_SWITCH	= 0x07090000 + 0x204,
};

uint32_t efuse_read(uint32_t offset);

void efuse_write(uint32_t offset, uint32_t value);



#endif // _SYS_SID_H_