# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
DT2C_DRIVER_SOURCES := drivers/remoteproc/rproc-sun20iw1.c \
	drivers/remoteproc/rproc-sun300iw1.c \
	drivers/remoteproc/rproc-sun50iw10.c \
	drivers/remoteproc/rproc-sun55iw3.c \
	drivers/remoteproc/rproc-sun8iw20.c \
	drivers/remoteproc/rproc-sun8iw21.c drivers/rtc/rtc.c
DT2C_BINDINGS := dts/bindings/remoteproc/allwinner,sunxi-remoteproc.yaml \
	dts/bindings/rtc/allwinner,sunxi-rtc.yaml

include ../../mk/dt2c-host.mk
