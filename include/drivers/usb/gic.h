/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __GIC_H__
#define __GIC_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define AW_IRQ_USB_OTG 61

void gic_enable(uint32_t irq);

int gic_is_pending(uint32_t irq);

#endif// __GIC_H__