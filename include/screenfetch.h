/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SCREENFETCH_H__
#define __SCREENFETCH_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Board information shown by screenfetch().
 *
 * A board fills what it knows in board_fetch_info(); empty fields are not printed.
 */
typedef struct {
	const char *model;	/* board name */
	const char *soc;	/* SoC name */
	const char *cpu;	/* core description */
	uint32_t mem_kb;	/* external memory size in KiB, 0: unknown */
	uint32_t sid[4];	/* chip id */
	int has_sid;
} screenfetch_info_t;

/** @brief Weak hook, the default leaves @p info empty. */
void board_fetch_info(screenfetch_info_t *info);

/**
 * @brief Print the logo with system information next to it, in the style of fastfetch.
 * @param[in] show_logo False prints the information only.
 *
 * Plain printf() output with ANSI colors (24 bit colors and half block characters for the logo), so it shows in
 * color on the UART and on the vt console.
 */
void screenfetch(bool show_logo);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* __SCREENFETCH_H__ */
