# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CPPFLAGS := -DCONFIG_DRIVER_MMC_TUNING=1 -Dmalloc=syter_malloc \
	-Drealloc=syter_realloc -Dfree=syter_free
TEST_SRCS = $(srctree)/lib/malloc.c

include ../../mk/host.mk
