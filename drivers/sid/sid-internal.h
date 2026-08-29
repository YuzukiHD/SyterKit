/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sid-internal.h
 * @brief Internal SID framework declarations.
 *
 * Shared by the per-SoC SID section layout files and the SID core driver.
 */

#ifndef __SUNXI_SID_INTERNAL_H__
#define __SUNXI_SID_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @struct sunxi_sid_section
 * @brief Describes one named eFuse storage section.
 *
 * Each SoC provides a table of these entries that the SID framework uses to
 * expose the eFuse regions by name.
 */
typedef struct sunxi_sid_section {
	const char *name;   /**< Section name used by the framework. */
	uint32_t offset;    /**< Byte offset of the section in eFuse space. */
	uint32_t size_bits; /**< Size of the section in bits. */
} sunxi_sid_section_t;

/**
 * @brief Table of named eFuse sections for the current SoC.
 */
extern const sunxi_sid_section_t sunxi_sid_sections[];

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
extern const size_t sunxi_sid_section_count;

#endif /* __SUNXI_SID_INTERNAL_H__ */
