# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_MMC)

obj-y += mmc-generic.o
obj-$(CONFIG_SOC_SUN300IW1) += mmc-sun300iw1.o
obj-$(CONFIG_SOC_SUN55IW3) += mmc-sun55iw3.o
obj-$(CONFIG_SOC_SUN55IW6) += mmc-sun55iw6.o
obj-$(CONFIG_SOC_SUN8IW22) += mmc-sun8iw22.o
