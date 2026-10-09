# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_SRCS = $(srctree)/core/cli/commands.c $(srctree)/lib/string.c \
	$(srctree)/lib/stdlib.c $(srctree)/lib/ctype.c \
	$(addprefix $(srctree)/core/cli/cmd/,help.c echo.c history.c hexdump.c read32.c write32.c ls.c)

include ../../mk/host.mk
