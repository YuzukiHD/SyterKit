/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "sid-sun252iw1: " fmt

/**
 * @file sid-sun252iw1.c
 * @brief Sun252iw1 eFuse SID section layout.
 *
 * Defines the named eFuse storage sections available on this SoC.
 */

#include "sid-platform.h"

/**
 * @brief eFuse section layout for the Sun252iw1 SoC.
 *
 * This SoC exposes no named eFuse sections, so the table is empty.
 */
const sunxi_sid_section_t sunxi_sid_sections[] = { { 0 } };

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
const size_t sunxi_sid_section_count = 0;
