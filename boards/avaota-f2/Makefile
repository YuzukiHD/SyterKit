# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-sram-y := app
apps-efex-y := init_dram
apps-dram-y := hello_world

board-libs-$(CONFIG_ARCH_CPU_E907) := lib/libdram-e907.a

ifeq ($(CONFIG_ARCH_CPU_C907),y)
board-libs-$(CONFIG_ARCH_RISCV32) := lib/libdram-c907-rv32.a
board-libs-$(CONFIG_ARCH_RISCV64) := lib/libdram-c907-rv64.a
endif

obj-$(CONFIG_ARCH_BOOT) += arch_boot/arch_boot.o
