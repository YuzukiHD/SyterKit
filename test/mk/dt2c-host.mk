# SPDX-License-Identifier: GPL-2.0+

.DEFAULT_GOAL := all

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
TEST_OUT ?= $(TEST_ROOT)/out
objtree := $(abspath $(TEST_OUT))

include $(srctree)/scripts/Makefile.dt2c

TEST_CFLAGS += -O2
TEST_CPPFLAGS += -DCONFIG_SOC_SUN8IW20 -I$(DT2C_INCLUDE) \
	-I$(srctree)/dts/include -I$(TEST_OUT)/$(notdir $(CURDIR))/include

include $(TEST_ROOT)/mk/host.mk

DT_HEADER := $(BUILD_DIR)/include/generated/fdt_generated.h
DT_DEPFILE := $(BUILD_DIR)/devicetree.d
DT_REPORT := $(BUILD_DIR)/devicetree.json
DT_DRIVER_MANIFEST := $(BUILD_DIR)/selected-drivers
dt2c_test_driver_sources := $(addprefix $(srctree)/,$(DT2C_DRIVER_SOURCES))
dt2c_test_binding_files := $(addprefix $(srctree)/,$(DT2C_BINDINGS))

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(DT_DEPFILE)
endif

$(TEST_BINARY): $(DT_HEADER)

$(DT_DRIVER_MANIFEST): $(CURDIR)/Makefile $(TEST_ROOT)/mk/dt2c-host.mk \
		$(dt2c_test_driver_sources)
	@mkdir -p $(dir $@)
	@printf '%s\n' $(dt2c_test_driver_sources) > $@

$(DT_HEADER): $(CURDIR)/Makefile $(TEST_ROOT)/mk/dt2c-host.mk \
		$(CURDIR)/board.dts $(DT_DRIVER_MANIFEST) $(dt2c_path) \
		$(dt2c_test_binding_files) | dt2c-check
	@mkdir -p $(dir $@)
	@echo "  DT2C   test/$(CASE_NAME)"
	@$(dt2c_path) generate --dts $(CURDIR)/board.dts \
		--bindings $(srctree)/dts/bindings \
		--drivers $(DT_DRIVER_MANIFEST) \
		--header $(DT_HEADER) --depfile $(DT_DEPFILE) \
		--report $(DT_REPORT) -I $(srctree)/dts/include
