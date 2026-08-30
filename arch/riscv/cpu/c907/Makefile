# SPDX-License-Identifier: GPL-2.0+

obj-$(CONFIG_APP_SRAM) += start.o
obj-$(CONFIG_APP_DRAM) += start.o
obj-y += cache.o

ifeq ($(CONFIG_ARCH_RISCV64),y)
ifeq ($(CONFIG_ARCH_RISCV64_ABI_LP64),y)
c907_abi := lp64
else
c907_abi := lp64d
endif
KBUILD_CFLAGS += -mcpu=c907fdv -mabi=$(c907_abi) \
	-mcmodel=medlow -mstrict-align -mtune=c907 \
	$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
KBUILD_AFLAGS += -mcpu=c907fdv -mabi=$(c907_abi) \
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
