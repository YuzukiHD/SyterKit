/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DRIVERS_DISPLAY_SUN252IW2_H__
#define __DRIVERS_DISPLAY_SUN252IW2_H__

/* Clock source ids of the sun252iw2 display blocks, used in the module clock parent tables. */
enum {
	SUN252IW2_DISP_SRC_HOSC = 1,
	SUN252IW2_DISP_SRC_PLL_PERI_1X, /* PLL_PERI(2X) / 2 */
	SUN252IW2_DISP_SRC_PLL_PERI_2X,
	SUN252IW2_DISP_SRC_PLL_VIDEO_1X, /* PLL_VIDEO / 4 */
	SUN252IW2_DISP_SRC_PLL_VIDEO_4X,
};

#endif
