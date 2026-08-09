# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
DT2C_DRIVER_SOURCES := drivers/gpio/gpio-v2.c drivers/i2c/i2c.c \
	drivers/pmu/axp1530.c \
	drivers/pmu/axp2202.c
DT2C_BINDINGS := dts/bindings/gpio/allwinner,sunxi-pinctrl.yaml \
	dts/bindings/i2c/allwinner,sunxi-i2c.yaml \
	dts/bindings/pmic/x-powers,axp-pmu.yaml

include ../../mk/dt2c-host.mk
