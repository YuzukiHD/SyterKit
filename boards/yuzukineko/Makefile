# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-y := app

board-libs-$(CONFIG_DRIVER_PSRAM) := lib/libpsram.a

obj-$(CONFIG_ARCH_BOOT) += arch_boot/arch_boot.o
