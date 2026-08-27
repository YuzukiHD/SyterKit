# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= riscv64-unknown-elf-

# CPU-core directories double as include roots (e.g. <e907/sysmap.h>).
KBUILD_CPPFLAGS += -I$(srctree)/arch/riscv/cpu

ifeq ($(CONFIG_BACKTRACE),y)
KBUILD_CFLAGS += -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

ifeq ($(CONFIG_ARCH_CPU_E907),y)
	KBUILD_CFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_AFLAGS += $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_LDFLAGS += -m elf32lriscv
endif

ifeq ($(CONFIG_ARCH_CPU_C907),y)
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
	KBUILD_CFLAGS += -mcpu=c907fdv-rv32 -mabi=ilp32d \
		-mcmodel=medlow -mstrict-align -mtune=c907 \
		$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_AFLAGS += -mcpu=c907fdv-rv32 -mabi=ilp32d \
		-mcmodel=medlow -mstrict-align -mtune=c907 \
		$(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_LDFLAGS += -m elf32lriscv
endif
endif

ifeq ($(CONFIG_ARCH_CPU_C906),y)
	KBUILD_CFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_AFLAGS += -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d \
	-mcmodel=medlow -mstrict-align $(call unquote,$(CONFIG_ARCH_EXTRA_CFLAG))
	KBUILD_LDFLAGS += -m elf64lriscv
endif
