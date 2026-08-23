# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_INTC)
obj-$(CONFIG_DRIVER_CLIC) += clic.o
obj-$(CONFIG_DRIVER_GIC) += gic.o
