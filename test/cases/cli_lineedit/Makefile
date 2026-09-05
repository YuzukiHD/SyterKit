# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CPPFLAGS += -DCONFIG_CLI_HEAP_STORAGE
TEST_SRCS = $(srctree)/core/cli/lineedit.c \
	$(srctree)/core/cli/history.c $(srctree)/lib/string.c \
	$(srctree)/lib/ctype.c

include ../../mk/host.mk
