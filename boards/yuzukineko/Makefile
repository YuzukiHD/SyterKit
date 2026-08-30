# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-sram-y := app
apps-efex-y := hello_world

board-libs-$(CONFIG_DRIVER_PSRAM) := lib/libpsram.a

obj-$(CONFIG_ARCH_BOOT) += arch_boot/arch_boot.o
