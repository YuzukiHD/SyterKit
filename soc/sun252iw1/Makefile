# SPDX-License-Identifier: GPL-2.0+

apps-efex-y := init_dram
soc-libs-$(CONFIG_ARCH_CPU_E907) := lib/libdram-e907.a

ifeq ($(CONFIG_ARCH_CPU_C907),y)
soc-libs-$(CONFIG_ARCH_RISCV32) := lib/libdram-c907-rv32.a
soc-libs-$(CONFIG_ARCH_RISCV64) := lib/libdram-c907-rv64.a
endif
