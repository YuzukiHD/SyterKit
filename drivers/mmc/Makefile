# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_MMC)
obj-y += core.o sdhci.o sdcard.o
obj-$(CONFIG_DRIVER_MMC_TUNING) += hs-timing.o tuning.o
obj-y += host/
