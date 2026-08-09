# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= riscv64-unknown-elf-

ifeq ($(CONFIG_BACKTRACE),y)
KBUILD_CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

ifeq ($(CONFIG_ARCH_RISCV32_CORE_E907),y)
KBUILD_CFLAGS += -mcpu=e907f -DXCFG_FORMAT_FLOAT=0
KBUILD_AFLAGS += -mcpu=e907f
KBUILD_LDFLAGS += -m elf32lriscv
endif

ifeq ($(CONFIG_ARCH_RISCV64_CORE_C906),y)
KBUILD_CFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mtune=c906 -mcmodel=medlow -mstrict-align
KBUILD_AFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mtune=c906 -mcmodel=medlow -mstrict-align
KBUILD_LDFLAGS += -m elf64lriscv
endif
