# SPDX-License-Identifier: GPL-2.0+

TEST_ARCH_FLAGS := -mcpu=cortex-a7 -mno-unaligned-access
TEST_EXTRA_CPPFLAGS := -DCONFIG_CPU_CORTEX_A7=1 -DCONFIG_ARCH_EXCEPTION=1
QEMU_SYSTEM := qemu-system-arm
QEMU_CPU := cortex-a7

include ../../mk/qemu-arm-startup.mk
