/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUNXI_SID_INTERNAL_H__
#define __SUNXI_SID_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>

typedef struct sunxi_sid_section {
	const char *name;
	uint32_t offset;
	uint32_t size_bits;
} sunxi_sid_section_t;

extern const sunxi_sid_section_t sunxi_sid_sections[];
extern const size_t sunxi_sid_section_count;

#endif /* __SUNXI_SID_INTERNAL_H__ */
