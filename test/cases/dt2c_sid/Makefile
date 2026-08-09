# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CPPFLAGS += -I$(srctree)/drivers/soc
TEST_SRCS = $(srctree)/drivers/soc/sid.c \
	$(srctree)/drivers/soc/sid-sun8iw21.c
DT2C_DRIVER_SOURCES := drivers/soc/sid.c
DT2C_BINDINGS := dts/bindings/soc/allwinner,sunxi-sid.yaml

include ../../mk/dt2c-host.mk
