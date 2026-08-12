# SPDX-License-Identifier: GPL-2.0+

TEST_ARCH_FLAGS := -march=armv8.2-a
TEST_EXTRA_CPPFLAGS := -DCONFIG_CPU_ARMV8_2=1
QEMU_SYSTEM := qemu-system-aarch64
QEMU_CPU := max
TEST_AARCH64_TRANSITION := 1

include ../../mk/qemu-arm-startup.mk
