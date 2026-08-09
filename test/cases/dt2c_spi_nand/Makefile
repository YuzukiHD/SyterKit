# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
DT2C_DRIVER_SOURCES := drivers/gpio/gpio-v2.c drivers/spi/spi.c \
	drivers/mtd/spi-nand.c
DT2C_BINDINGS := dts/bindings/gpio/allwinner,sunxi-pinctrl.yaml \
	dts/bindings/spi/allwinner,sunxi-spi.yaml \
	dts/bindings/mtd/spi-nand.yaml

include ../../mk/dt2c-host.mk
