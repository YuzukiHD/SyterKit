# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CPPFLAGS := -Dmalloc=syter_malloc -Drealloc=syter_realloc \
	-Dfree=syter_free
TEST_SRCS = $(srctree)/lib/malloc.c

include ../../mk/host.mk
