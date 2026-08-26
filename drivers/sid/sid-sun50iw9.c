/* SPDX-License-Identifier: GPL-2.0+ */

#include "sid-internal.h"

const sunxi_sid_section_t sunxi_sid_sections[] = {
	{ "chipid", 0x0000, 128 },
	{ "brom-conf", 0x0010, 32 },
	{ "thermal-sensor", 0x0014, 64 },
	{ "ft-zone", 0x001c, 128 },
	{ "oem", 0x002c, 160 },
	{ "write-protect", 0x0040, 32 },
	{ "read-protect", 0x0044, 32 },
	{ "jtag-lcjs", 0x0048, 32 },
	{ "jtag-attr", 0x004c, 32 },
	{ "efuse-huk", 0x0050, 192 },
	{ "efuse-ind", 0x0068, 32 },
	{ "efuse-id", 0x006c, 32 },
	{ "rotpk", 0x0070, 256 },
	{ "ssk", 0x0090, 128 },
	{ "rssk", 0x00a0, 256 },
	{ "sn", 0x00b0, 192 },
	{ "nv1", 0x00c8, 32 },
	{ "nv2", 0x00cc, 32 },
	{ "hdcp-hash", 0x00d0, 128 },
	{ "backup-key0", 0x00e0, 192 },
	{ "backup-key1", 0x00f8, 72 },
};

const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
