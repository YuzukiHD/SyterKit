# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_SID)
obj-y += sid.o
obj-$(CONFIG_DRIVER_SID) += platform/
