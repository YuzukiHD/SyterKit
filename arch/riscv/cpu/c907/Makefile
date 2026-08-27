# SPDX-License-Identifier: GPL-2.0+

obj-y += cache.o

ifeq ($(CONFIG_ARCH_RISCV64),y)
KBUILD_CFLAGS += -mcpu=c907fdv -mabi=lp64d \
	-mcmodel=medlow -mstrict-align -mtune=c907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -mcpu=c907fdv -mabi=lp64d \
	-mcmodel=medlow -mstrict-align -mtune=c907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf64lriscv
endif
ifeq ($(CONFIG_ARCH_RISCV32),y)
KBUILD_CFLAGS += -mcpu=c907fdv-rv32 -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=c907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -mcpu=c907fdv-rv32 -mabi=ilp32 \
	-mcmodel=medlow -mstrict-align -mtune=c907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_LDFLAGS += -m elf32lriscv
endif
