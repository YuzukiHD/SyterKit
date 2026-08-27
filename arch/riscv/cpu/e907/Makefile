# SPDX-License-Identifier: GPL-2.0+

obj-y += cache.o

KBUILD_CFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf32lriscv
