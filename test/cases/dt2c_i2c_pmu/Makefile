# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_SRCS := $(CURDIR)/../../../drivers/pmu/axp1530.c \
	$(CURDIR)/../../../drivers/pmu/axp2202.c
TEST_CFLAGS += -ffunction-sections
TEST_LDFLAGS += -Wl,--gc-sections
DT2C_DRIVER_SOURCES := drivers/gpio/gpio-v2.c drivers/i2c/i2c.c
DT2C_BINDINGS := dts/bindings/gpio/allwinner,sunxi-pinctrl.yaml \
	dts/bindings/i2c/allwinner,sunxi-i2c.yaml

include ../../mk/dt2c-host.mk
