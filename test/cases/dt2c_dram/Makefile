# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_SRCS = $(srctree)/drivers/dram/dram.c
DT2C_DRIVER_SOURCES := drivers/dram/dram-sun20iw1.c \
	drivers/dram/dram-sun252iw1.c drivers/dram/dram-sun300iw1.c \
	drivers/dram/dram-sun50iw9.c drivers/dram/dram-sun50iw10.c \
	drivers/dram/dram-sun55iw3.c drivers/dram/dram-sun55iw6.c \
	drivers/dram/dram-sun60iw2.c drivers/dram/dram-sun65iw1.c \
	drivers/dram/dram-sun8iw20.c drivers/dram/dram-sun8iw21.c \
	drivers/dram/dram-sun8iw22.c drivers/gpio/gpio-v2.c \
	drivers/i2c/i2c.c \
	drivers/pmu/axp1530.c drivers/pmu/axp2202.c drivers/rtc/rtc.c
DT2C_BINDINGS := dts/bindings/gpio/allwinner,sunxi-pinctrl.yaml \
	dts/bindings/i2c/allwinner,sunxi-i2c.yaml \
	dts/bindings/memory/allwinner,sunxi-dram.yaml \
	dts/bindings/pmic/x-powers,axp-pmu.yaml \
	dts/bindings/rtc/allwinner,sunxi-rtc.yaml

include ../../mk/dt2c-host.mk
