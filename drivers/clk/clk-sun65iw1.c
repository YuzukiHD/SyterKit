/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/clk/sun65iw1/reg.h>
#include <dt2c/driver.h>

void sunxi_clk_init(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void sunxi_clk_reset(sunxi_ccu_t *ccu) {
	(void) ccu;
}

DT2C_DRIVER_COMPAT("allwinner,sun65iw1-ccu");
