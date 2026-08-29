/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sid-sun20iw1.c
 * @brief Sun20iw1 eFuse SID section layout.
 *
 * Defines the named eFuse storage sections available on this SoC.
 */

#include "sid-internal.h"

/**
 * @brief eFuse section layout for the Sun20iw1 SoC.
 *
 * Each entry names an eFuse region, its byte offset and its size in bits.
 */
const sunxi_sid_section_t sunxi_sid_sections[] = {
	{ "chipid", 0x0000, 128 },
	{ "brom-conf-try", 0x0010, 32 },
	{ "thermal-sensor", 0x0014, 64 },
	{ "ft-zone", 0x001c, 128 },
	{ "tvout", 0x002c, 32 },
	{ "tvout-gamma", 0x0030, 64 },
	{ "oem-program", 0x0038, 64 },
	{ "write-protect", 0x0040, 32 },
	{ "read-protect", 0x0044, 32 },
	{ "reserved1", 0x0048, 64 },
	{ "huk", 0x0050, 192 },
	{ "reserved2", 0x0068, 64 },
	{ "rotpk", 0x0070, 256 },
	{ "ssk", 0x0090, 256 },
	{ "rssk", 0x00b0, 128 },
	{ "hdcp-hash", 0x00c0, 128 },
	{ "nv1", 0x00d0, 32 },
	{ "nv2", 0x00d4, 32 },
	{ "reserved3", 0x00d8, 96 },
	{ "oem-program-secure", 0x00e4, 224 },
};

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
