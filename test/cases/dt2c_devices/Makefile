# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_CFLAGS += -O2

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
TEST_OUT ?= $(TEST_ROOT)/out
objtree := $(abspath $(TEST_OUT))
include $(srctree)/scripts/Makefile.dt2c

TEST_CPPFLAGS += -DCONFIG_SOC_SUN8IW20 -I$(DT2C_INCLUDE) \
	-I$(srctree)/dts/include \
	-I$(TEST_OUT)/$(notdir $(CURDIR))/include

include ../../mk/host.mk

DT_HEADER := $(BUILD_DIR)/include/generated/fdt_generated.h
DT_DEPFILE := $(BUILD_DIR)/devicetree.d
DT_REPORT := $(BUILD_DIR)/devicetree.json
DT_DRIVER_MANIFEST := $(BUILD_DIR)/selected-drivers
DT_DRIVER_SOURCES := $(srctree)/drivers/serial/serial.c \
	$(srctree)/drivers/dma/dma.c \
	$(srctree)/drivers/i2c/i2c.c \
	$(srctree)/drivers/pmu/axp1530.c \
	$(srctree)/drivers/pmu/axp2101.c \
	$(srctree)/drivers/pmu/axp2202.c \
	$(srctree)/drivers/pmu/axp333.c \
	$(srctree)/drivers/pmu/axp8191.c \
	$(srctree)/drivers/spi/spi.c

-include $(DT_DEPFILE)

$(TEST_BINARY): $(DT_HEADER)

$(DT_DRIVER_MANIFEST): $(DT_DRIVER_SOURCES)
	@mkdir -p $(dir $@)
	@printf '%s\n' $(DT_DRIVER_SOURCES) > $@

$(DT_HEADER): $(CURDIR)/board.dts $(DT_DRIVER_MANIFEST) $(dt2c_path) \
		$(srctree)/dts/bindings/serial/allwinner,sunxi-uart.yaml \
		$(srctree)/dts/bindings/dma/allwinner,sunxi-dma.yaml \
		$(srctree)/dts/bindings/i2c/allwinner,sunxi-i2c.yaml \
		$(srctree)/dts/bindings/pmic/x-powers,axp-pmu.yaml \
		$(srctree)/dts/bindings/spi/allwinner,sunxi-spi.yaml

$(DT_HEADER): | dt2c-check
	@mkdir -p $(dir $@)
	@echo "  DT2C   test/dt2c_devices"
	@$(dt2c_path) generate --dts $(CURDIR)/board.dts \
		--bindings $(srctree)/dts/bindings \
		--drivers $(DT_DRIVER_MANIFEST) \
		--header $(DT_HEADER) --depfile $(DT_DEPFILE) \
		--report $(DT_REPORT) -I $(srctree)/dts/include
