# SPDX-License-Identifier: GPL-2.0+

obj-$(CONFIG_APP_SRAM) += start.o
obj-$(CONFIG_APP_DRAM) += start.o
obj-$(CONFIG_EFEX) += efex.o
obj-y += cache.o

KBUILD_CFLAGS += -mcpu=e907f -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=e907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -mcpu=e907f -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=e907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf32lriscv
