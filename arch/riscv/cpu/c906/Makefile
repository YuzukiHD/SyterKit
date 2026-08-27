# SPDX-License-Identifier: GPL-2.0+

obj-y += cache.o

KBUILD_CFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf64lriscv
