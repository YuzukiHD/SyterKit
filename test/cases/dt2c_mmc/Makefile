# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_SRCS = $(srctree)/drivers/mmc/host/mmc-sun8iw20.c
DT2C_DRIVER_SOURCES := drivers/gpio/gpio-v2.c drivers/mmc/sdhci.c
DT2C_BINDINGS := dts/bindings/gpio/allwinner,sunxi-pinctrl.yaml \
	dts/bindings/mmc/allwinner,sunxi-mmc.yaml

include ../../mk/dt2c-host.mk
