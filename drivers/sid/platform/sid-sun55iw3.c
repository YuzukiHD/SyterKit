/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "sid-sun55iw3: " fmt

/**
 * @file sid-sun55iw3.c
 * @brief Sun55iw3 eFuse SID section layout.
 *
 * Defines the named eFuse storage sections available on this SoC.
 */

#include "sid-platform.h"

/**
 * @brief eFuse section layout for the Sun55iw3 SoC.
 *
 * Each entry names an eFuse region, its byte offset and its size in bits.
 */
const sunxi_sid_section_t sunxi_sid_sections[] = {
	{ "chipid", 0x0000, 128 },
	{ "brom-config", 0x0010, 32 },
	{ "aldo-fix", 0x0014, 1 },
	{ "thermal-sensor", 0x0030, 64 },
	{ "tf-zone", 0x0028, 128 },
	{ "oem-program", 0x003c, 160 },
	{ "write-protect", 0x0080, 32 },
	{ "read-protect", 0x0084, 32 },
	{ "lcjs", 0x0088, 32 },
	{ "attr", 0x0090, 32 },
	{ "huk", 0x0094, 192 },
	{ "reserved1", 0x00ac, 64 },
	{ "rotpk", 0x00b4, 256 },
	{ "ssk", 0x00d4, 128 },
	{ "rssk", 0x00f4, 256 },
	{ "sn", 0x00b0, 192 },
	{ "nv1", 0x0124, 32 },
	{ "nv2", 0x0128, 32 },
	{ "hdcp-hash", 0x0114, 128 },
	{ "backup-key", 0x0164, 192 },
	{ "backup-key2", 0x01a4, 72 },
};

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
