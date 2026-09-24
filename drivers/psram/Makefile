# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_PSRAM)

obj-y += psram.o
obj-$(CONFIG_SOC_SUN252IW2) += psram-sun252iw2.o
