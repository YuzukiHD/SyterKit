/* SPDX-License-Identifier: GPL-2.0+ */

#include "sid-internal.h"

const sunxi_sid_section_t sunxi_sid_sections[] = {
	{ "chipid", 0x0000, 128 },
	{ "brom-conf-try", 0x0010, 32 },
	{ "thermal-sensor", 0x0014, 64 },
	{ "ft-zone", 0x001c, 128 },
	{ "reserved1", 0x002c, 96 },
	{ "write-protect", 0x0038, 32 },
	{ "read-protect", 0x003c, 32 },
	{ "lcjs", 0x0040, 32 },
	{ "reserved2", 0x0044, 800 },
	{ "rotpk", 0x00a8, 256 },
	{ "reserved3", 0x00c8, 448 },
};

const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
