# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= riscv64-unknown-elf-

ifeq ($(CONFIG_BACKTRACE),y)
KBUILD_CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

ifeq ($(CONFIG_ARCH_RISCV32_CORE_E907),y)
KBUILD_CFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf32lriscv
endif

ifeq ($(CONFIG_ARCH_RISCV64_CORE_C906),y)
	KBUILD_CFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_AFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_LDFLAGS += -m elf64lriscv
endif
