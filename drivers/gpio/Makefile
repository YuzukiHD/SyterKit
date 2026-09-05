# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_GPIO)
obj-$(CONFIG_DRIVER_GPIO_V1) += gpio-v1.o
obj-$(CONFIG_DRIVER_GPIO_V2) += gpio-v2.o
obj-$(CONFIG_DRIVER_GPIO_V2_POW) += gpio-v2.o gpio-v2-pow.o
obj-$(CONFIG_DRIVER_GPIO_V3) += gpio-v3.o
obj-$(CONFIG_DRIVER_GPIO_V4) += gpio-v4.o
