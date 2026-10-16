/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DRIVERS_DISPLAY_SUN252IW1_H__
#define __DRIVERS_DISPLAY_SUN252IW1_H__

/* Clock source ids of the sun252iw1 (V861/V881) display blocks, used in the module clock parent tables. */
enum {
	SUN252IW1_DISP_SRC_HOSC = 1,
	SUN252IW1_DISP_SRC_PLL_PERI_2X,
	SUN252IW1_DISP_SRC_PLL_PERI_300M, /* PLL_PERI(2X) / 4 */
	SUN252IW1_DISP_SRC_PLL_VIDEO_1X,  /* PLL_VIDEO / 4 */
	SUN252IW1_DISP_SRC_PLL_VIDEO_4X,  /* PLL_VIDEO / (M + 1), M = 0 */
};

#endif
