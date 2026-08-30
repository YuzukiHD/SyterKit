# SPDX-License-Identifier: GPL-2.0+

obj-y += cache.o

KBUILD_CFLAGS += -mcpu=e907f -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=e907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -mcpu=e907f -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=e907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf32lriscv
