# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk

ccflags-y += $(call driver-log-flags,DRIVER_UFS)
obj-y += generic.o
obj-y += noop.o
obj-$(CONFIG_DRIVER_UFS_HOST_SUN60IW2) += sun60iw2.o
