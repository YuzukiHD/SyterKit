/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sid-sun8iw22.c
 * @brief Sun8iw22 eFuse SID section layout.
 *
 * Defines the named eFuse storage sections available on this SoC.
 */

#include "sid-internal.h"

/**
 * @brief eFuse section layout for the Sun8iw22 SoC.
 *
 * This SoC exposes no named eFuse sections, so the table is empty.
 */
const sunxi_sid_section_t sunxi_sid_sections[] = { { 0 } };

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
const size_t sunxi_sid_section_count = 0;
