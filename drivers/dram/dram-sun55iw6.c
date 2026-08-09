/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <mmu.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <dt2c/driver.h>
#include <drivers/reg/reg-ncat.h>
#include <drivers/dram.h>
#include <drivers/rtc.h>

DT2C_DRIVER_COMPAT("allwinner,sun55iw6-dram");
