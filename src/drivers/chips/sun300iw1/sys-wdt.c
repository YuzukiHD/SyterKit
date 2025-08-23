/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

void sys_reset() {
	setbits_le32(SUNXI_PRCM_BASE + 0x1c, BIT(3));			/* enable WDT clk */
	writel(0x16aa0000, SUNXI_RTC_WDG_BASE + 0x18);			/* disable WDT */
	writel(0x16aa0000 | BIT(0), SUNXI_RTC_WDG_BASE + 0x08); /* trigger WDT */
}