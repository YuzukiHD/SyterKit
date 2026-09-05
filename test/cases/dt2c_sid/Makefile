# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CPPFLAGS += -I$(srctree)/drivers/sid -I$(srctree)/drivers/sid/platform
TEST_SRCS = $(srctree)/drivers/sid/sid.c \
	$(srctree)/drivers/sid/platform/sid-platform.c \
	$(srctree)/drivers/sid/platform/sid-sun8iw21.c
DT2C_DRIVER_SOURCES := drivers/sid/sid.c
DT2C_BINDINGS := dts/bindings/soc/allwinner,sunxi-sid.yaml

include ../../mk/dt2c-host.mk
