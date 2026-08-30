# SPDX-License-Identifier: GPL-2.0+

obj-$(CONFIG_APP_SRAM) += start.o
obj-$(CONFIG_APP_DRAM) += start.o
obj-$(CONFIG_EFEX) += efex.o
obj-y += cache.o

KBUILD_CFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf64lriscv
