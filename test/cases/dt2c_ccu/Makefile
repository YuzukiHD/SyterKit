# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
DT2C_DRIVER_SOURCES := drivers/clk/clk-sun20iw1.c \
	drivers/clk/clk-sun252iw1.c \
	drivers/clk/clk-sun300iw1.c \
	drivers/clk/clk-sun50iw9.c \
	drivers/clk/clk-sun50iw10.c \
	drivers/clk/clk-sun55iw3.c \
	drivers/clk/clk-sun55iw6.c \
	drivers/clk/clk-sun60iw2.c \
	drivers/clk/clk-sun65iw1.c \
	drivers/clk/clk-sun8iw20.c \
	drivers/clk/clk-sun8iw21.c \
	drivers/clk/clk-sun8iw22.c
DT2C_BINDINGS := dts/bindings/clock/allwinner,sun20iw1-ccu.yaml \
	dts/bindings/clock/allwinner,sun252iw1-ccu.yaml \
	dts/bindings/clock/allwinner,sun300iw1-ccu.yaml \
	dts/bindings/clock/allwinner,sun50iw9-ccu.yaml \
	dts/bindings/clock/allwinner,sun50iw10-ccu.yaml \
	dts/bindings/clock/allwinner,sun55iw3-ccu.yaml \
	dts/bindings/clock/allwinner,sun55iw6-ccu.yaml \
	dts/bindings/clock/allwinner,sun60iw2-ccu.yaml \
	dts/bindings/clock/allwinner,sun65iw1-ccu.yaml \
	dts/bindings/clock/allwinner,sun8iw20-ccu.yaml \
	dts/bindings/clock/allwinner,sun8iw21-ccu.yaml \
	dts/bindings/clock/allwinner,sun8iw22-ccu.yaml

include ../../mk/dt2c-host.mk

INVALID_REG_NAMES_OUT := $(BUILD_DIR)/invalid-reg-names

.PHONY: validate-invalid-reg-names

run: validate-invalid-reg-names

validate-invalid-reg-names: $(CURDIR)/invalid-reg-names.dts \
		$(DT_DRIVER_MANIFEST) $(dt2c_path) | dt2c-check
	@mkdir -p $(INVALID_REG_NAMES_OUT)/include/generated
	@if $(dt2c_path) generate \
		--dts $(CURDIR)/invalid-reg-names.dts \
		--bindings $(srctree)/dts/bindings \
		--drivers $(DT_DRIVER_MANIFEST) \
		--header $(INVALID_REG_NAMES_OUT)/include/generated/fdt_generated.h \
		--depfile $(INVALID_REG_NAMES_OUT)/devicetree.d \
		--report $(INVALID_REG_NAMES_OUT)/devicetree.json \
		-I $(srctree)/dts/include >/dev/null 2>&1; then \
		echo "invalid CCU reg-names unexpectedly passed validation" >&2; \
		exit 1; \
	fi
