# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= riscv64-unknown-elf-

# CPU-core directories double as include roots (e.g. <e907/sysmap.h>).
KBUILD_CPPFLAGS += -I$(srctree)/arch/riscv/cpu

ifeq ($(CONFIG_BACKTRACE),y)
KBUILD_CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

# Each CPU core owns its toolchain flags and object list in
# arch/riscv/cpu/<core>/Makefile.  Pull the selected core's flags up into the
# top-level Kbuild so they apply to the whole image; the obj-y it defines is
# only consumed when Makefile.build recurses into the core directory.
ifeq ($(CONFIG_ARCH_CPU_E907),y)
include $(srctree)/arch/riscv/cpu/e907/Makefile
endif
ifeq ($(CONFIG_ARCH_CPU_C907),y)
include $(srctree)/arch/riscv/cpu/c907/Makefile
endif
ifeq ($(CONFIG_ARCH_CPU_C906),y)
include $(srctree)/arch/riscv/cpu/c906/Makefile
endif
