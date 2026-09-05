# SPDX-License-Identifier: GPL-2.0+

TEST_ARCH_FLAGS := -march=armv8-a
TEST_EXTRA_CPPFLAGS := -DCONFIG_CPU_ARMV8=1 -DCONFIG_ARCH_EXCEPTION=1
QEMU_SYSTEM := qemu-system-aarch64
QEMU_CPU := cortex-a53
TEST_AARCH64_TRANSITION := 1

include ../../mk/qemu-arm-startup.mk
