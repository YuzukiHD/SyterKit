# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_SRCS = $(srctree)/core/initcall.c
TEST_CFLAGS := -ffunction-sections -fdata-sections
TEST_LDFLAGS := -Wl,--gc-sections -Wl,-T,$(CURDIR)/linker.ld

include ../../mk/host.mk
