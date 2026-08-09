# SPDX-License-Identifier: GPL-2.0+

VERSION := 0
PATCHLEVEL := 5
SUBLEVEL := 0

.DEFAULT_GOAL := all
MAKEFLAGS += --no-print-directory --silent

srctree := $(CURDIR)
ifneq ($(O),)
objtree := $(abspath $(O))
else
objtree := $(srctree)
endif

KCONFIG_CONFIG := $(objtree)/.config
auto_conf := $(objtree)/include/config/auto.conf
auto_header := $(objtree)/include/generated/autoconf.h
kconfig_out := $(objtree)/.kconfig
conf := $(kconfig_out)/conf
mconf := $(kconfig_out)/mconf
kconfig_sources := $(wildcard $(srctree)/scripts/kconfig/*.[ch] \
	$(srctree)/scripts/kconfig/*.y $(srctree)/scripts/kconfig/*.l \
	$(srctree)/scripts/kconfig/*.sh \
	$(srctree)/scripts/kconfig/lxdialog/*.[ch]) \
	$(srctree)/scripts/kconfig/Makefile
board_kconfig_list := $(kconfig_out)/boards.Kconfig
board_kconfigs := $(sort $(wildcard $(srctree)/boards/*/Kconfig))
BOARD_KCONFIG_LIST := $(board_kconfig_list)
kconfig_env := KCONFIG_CONFIG=$(KCONFIG_CONFIG) \
	KCONFIG_AUTOCONFIG=$(auto_conf) KCONFIG_AUTOHEADER=$(auto_header) \
	BOARD_KCONFIG_LIST=$(BOARD_KCONFIG_LIST)

no_dot_config_targets := config menuconfig olddefconfig syncconfig defconfig \
	savedefconfig %_defconfig clean mrproper help list-defconfigs check tools \
	utilities list-utilities test docs

ifeq ($(strip $(MAKECMDGOALS)),)
-include $(auto_conf)
-include $(auto_conf).cmd
else ifneq ($(strip $(filter-out $(no_dot_config_targets),$(MAKECMDGOALS))),)
-include $(auto_conf)
-include $(auto_conf).cmd
endif

unquote = $(subst ",,$(1))
board := $(call unquote,$(CONFIG_SYS_BOARD))

arch_dir := $(if $(CONFIG_ARCH_ARM32),arm,riscv)
arch_inc := arch/$(arch_dir)/include
include_dirs := include $(arch_inc)

KBUILD_CPPFLAGS := -I$(objtree)/include/generated \
	$(addprefix -I$(srctree)/,$(include_dirs)) \
	-I$(srctree)/dts/include -include $(auto_header)
KBUILD_CFLAGS := -nostdlib -nostdinc -ffreestanding -fno-builtin \
	-fno-common -fno-stack-protector -ffunction-sections -fdata-sections \
	-Wall
KBUILD_AFLAGS := $(KBUILD_CFLAGS)

ifeq ($(CONFIG_OPTIMIZE_FOR_SIZE),y)
KBUILD_CFLAGS += -Os
else
KBUILD_CFLAGS += -O2
endif
ifeq ($(CONFIG_BUILD_DEBUG),y)
KBUILD_CFLAGS += -g -ggdb
KBUILD_AFLAGS += -g -ggdb
KBUILD_CPPFLAGS += -DDEBUG_MODE
endif
ifeq ($(CONFIG_BUILD_TRACE),y)
KBUILD_CPPFLAGS += -DTRACE_MODE
endif

ifneq ($(board),)
include $(srctree)/arch/$(arch_dir)/Makefile
include $(srctree)/boards/$(board)/Makefile
endif

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif
ifeq ($(origin AR),default)
AR := $(CROSS_COMPILE)ar
endif
ifeq ($(origin LD),default)
LD := $(CROSS_COMPILE)ld
endif
NM ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
SIZE ?= $(CROSS_COMPILE)size
HOSTCC ?= cc
DOXYGEN ?= doxygen
include $(srctree)/scripts/Makefile.dt2c
FIRMWARE_CROSS_COMPILE ?= riscv64-unknown-elf-
BL33_CROSS_COMPILE ?= arm-none-eabi-

ifneq ($(board),)
board_dts := $(srctree)/boards/$(board)/board.dts
dt2c_bindings := $(srctree)/dts/bindings
dt2c_out := $(objtree)/.obj/boards/$(board)/dt2c
dt2c_driver_manifest := $(dt2c_out)/selected-drivers
dt2c_driver_sources := $(if $(filter y,$(CONFIG_DRIVER_SERIAL)),\
	$(srctree)/drivers/serial/serial.c) \
	$(if $(filter y,$(CONFIG_DRIVER_DMA)),\
	$(srctree)/drivers/dma/dma.c) \
	$(if $(filter y,$(CONFIG_DRIVER_I2C)),\
	$(srctree)/drivers/i2c/i2c.c) \
	$(if $(filter y,$(CONFIG_DRIVER_PMIC_AXP)),\
	$(srctree)/drivers/pmu/axp1530.c \
	$(srctree)/drivers/pmu/axp2101.c \
	$(srctree)/drivers/pmu/axp2202.c \
	$(srctree)/drivers/pmu/axp333.c \
	$(srctree)/drivers/pmu/axp8191.c) \
	$(if $(filter y,$(CONFIG_DRIVER_SPI)),\
	$(srctree)/drivers/spi/spi.c)
dt2c_include := $(dt2c_out)/include
dt2c_header := $(dt2c_include)/generated/fdt_generated.h
dt2c_depfile := $(dt2c_out)/devicetree.d
dt2c_report := $(dt2c_out)/devicetree.json
KBUILD_CPPFLAGS += -I$(dt2c_include)
KBUILD_CPPFLAGS += -I$(DT2C_INCLUDE)
-include $(dt2c_depfile)
endif

apps := $(apps-y)

export srctree objtree CC AR LD NM OBJCOPY OBJDUMP SIZE HOSTCC DT2C DT2C_INCLUDE
export KBUILD_CPPFLAGS KBUILD_CFLAGS KBUILD_AFLAGS KBUILD_LDFLAGS

.PHONY: all artifacts image images prepare tools utilities firmware check config \
	menuconfig olddefconfig syncconfig defconfig savedefconfig list-defconfigs \
	list-apps list-firmware list-utilities test docs help clean mrproper \
	board-kconfig dt2c-check FORCE

ifneq ($(wildcard $(auto_conf)),)
all: images
else
all:
	@echo "No configuration found. Run 'make <board>_defconfig' or 'make menuconfig'."
	@false
endif

$(conf) $(mconf): $(kconfig_sources)
	@$(MAKE) -C $(srctree)/scripts/kconfig O=$(kconfig_out) \
		HOSTCC="$(HOSTCC)" all

board-kconfig:
	@set -e; \
	mkdir -p $(dir $(board_kconfig_list)); \
	tmp=$(board_kconfig_list).tmp.$$$$; \
	{ for file in $(board_kconfigs); do \
		printf 'source "%s"\n' "$${file#$(srctree)/}"; \
	done; } > "$$tmp"; \
	if ! cmp -s "$$tmp" $(board_kconfig_list); then \
		mv "$$tmp" $(board_kconfig_list); \
	else \
		$(RM) "$$tmp"; \
	fi

config: $(conf) | board-kconfig
	@mkdir -p $(objtree)
	@$(kconfig_env) $(conf) $(srctree)/Kconfig
	@$(MAKE) O=$(O) syncconfig

menuconfig: $(mconf) | board-kconfig
	@mkdir -p $(objtree)
	@$(kconfig_env) $(mconf) $(srctree)/Kconfig
	@$(MAKE) O=$(O) syncconfig

olddefconfig: $(conf) | board-kconfig
	@$(kconfig_env) $(conf) --olddefconfig $(srctree)/Kconfig
	@$(MAKE) O=$(O) syncconfig

syncconfig: $(conf) | board-kconfig
	@mkdir -p $(dir $(auto_conf)) $(dir $(auto_header))
	@$(kconfig_env) $(conf) --syncconfig $(srctree)/Kconfig

$(auto_conf) $(auto_header) &: $(KCONFIG_CONFIG) $(conf) | board-kconfig
	@mkdir -p $(dir $(auto_conf)) $(dir $(auto_header))
	@$(kconfig_env) $(conf) --syncconfig $(srctree)/Kconfig

defconfig: tinyvision_defconfig

%_defconfig: $(conf) | board-kconfig
	@test -f $(srctree)/configs/$@ || { echo "Unknown defconfig: $@"; exit 1; }
	@mkdir -p $(objtree)
	@echo "  DEFCONFIG  $@"
	@$(kconfig_env) $(conf) \
		--defconfig=$(srctree)/configs/$@ $(srctree)/Kconfig
	@$(MAKE) O=$(O) syncconfig

savedefconfig: $(conf) | board-kconfig
	@$(kconfig_env) $(conf) \
		--savedefconfig=$(objtree)/defconfig $(srctree)/Kconfig

list-defconfigs:
	@cd $(srctree)/configs && printf '%s\n' *_defconfig

list-apps:
	@$(if $(apps),printf '%s\n' $(apps),:)

list-firmware:
	@$(if $(firmware-y),printf '%s\n' $(firmware-y),:)

utility_dirs := bl33 bl33_t527
utility_root := $(objtree)/utilities
utility_rel_root := utilities

list-utilities:
	@printf '%s\n' $(utility_dirs)

prepare: $(objtree)/include/generated/config.h $(dt2c_header)

git_hash := $(shell git -C $(srctree) rev-parse --short HEAD 2>/dev/null || echo unknown)
$(objtree)/include/generated/config.h: $(auto_header) $(srctree)/Makefile
	@mkdir -p $(dir $@)
	@echo "  GEN     $(patsubst $(objtree)/%,%,$@)"
	@{ \
		echo '/* SPDX-License-Identifier: GPL-2.0+ */'; \
		echo '#ifndef __GENERATED_CONFIG_H__'; \
		echo '#define __GENERATED_CONFIG_H__'; \
		echo '#define PROJECT_NAME "SyterKit"'; \
		echo '#define PROJECT_VERSION "$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)"'; \
		echo '#define PROJECT_GIT_HASH "$(git_hash)"'; \
		echo '#define PROJECT_C_COMPILER "$(notdir $(CC))"'; \
		echo '#define PROJECT_C_COMPILER_VERSION "'$$($(CC) -dumpfullversion -dumpversion)'"'; \
		echo '#endif'; \
	} > $@

ifneq ($(board),)
$(dt2c_driver_manifest): $(dt2c_driver_sources) $(auto_conf) \
		$(srctree)/Makefile
	@mkdir -p $(dir $@)
	@tmp=$@.tmp; : > $$tmp; \
	for source in $(dt2c_driver_sources); do \
		printf '%s\n' "$$source" >> $$tmp; \
	done; \
	if cmp -s $$tmp $@; then \
		$(RM) $$tmp; touch $@; \
	else \
		mv $$tmp $@; \
	fi

$(dt2c_header): $(board_dts) $(dt2c_driver_manifest) $(dt2c_path) \
		| dt2c-check
	@mkdir -p $(dir $@)
	@echo "  DT2C   boards/$(board)/board.dts"
	@$(dt2c_path) generate \
		--dts $(board_dts) \
		--bindings $(dt2c_bindings) \
		--drivers $(dt2c_driver_manifest) \
		--header $(dt2c_header) \
		--depfile $(dt2c_depfile) \
		--report $(dt2c_report) \
		-I $(srctree)/dts/include
endif

build_dirs := core lib drivers arch/$(arch_dir) boards/$(board)
common_builtins := $(addprefix $(objtree)/.obj/,$(addsuffix /built-in.o,$(build_dirs)))

$(objtree)/.obj/%/built-in.o: prepare FORCE
	@$(MAKE) -f $(srctree)/scripts/Makefile.build obj=$*

$(objtree)/.obj/apps/$(board)/%/built-in.o: prepare FORCE
	@$(MAKE) -f $(srctree)/scripts/Makefile.app board=$(board) app=$*

board_libs := $(addprefix $(srctree)/boards/$(board)/,$(board-libs-y))

image_root := $(objtree)/build/$(board)
image_rel_root := build/$(board)
backtrace_address_bits := $(if $(CONFIG_ARCH_RISCV64),64,32)
fel_lds := $(image_root)/link_fel.ld
bin_lds := $(image_root)/link_bin.ld

ifneq ($(board-fel-lds-y),)
board_fel_lds := $(srctree)/boards/$(board)/$(board-fel-lds-y)
board_bin_lds := $(srctree)/boards/$(board)/$(board-bin-lds-y)

$(fel_lds): $(board_fel_lds)
	@mkdir -p $(dir $@)
	@cp $< $@
$(bin_lds): $(board_bin_lds)
	@mkdir -p $(dir $@)
	@cp $< $@
else
lds_src := $(srctree)/arch/$(arch_dir)/syterkit.lds.S
$(fel_lds): $(lds_src) $(auto_header)
	@mkdir -p $(dir $@)
	@echo "  LDS     $(image_rel_root)/link_fel.ld"
	@$(CC) $(KBUILD_CPPFLAGS) -E -P -x c -DSPL_TEXT_BASE=$(CONFIG_SPL_FEL_TEXT_BASE) \
		-DSPL_MAX_SIZE=$(CONFIG_SPL_FEL_MAX_SIZE) $< -o $@
$(bin_lds): $(lds_src) $(auto_header)
	@mkdir -p $(dir $@)
	@echo "  LDS     $(image_rel_root)/link_bin.ld"
	@$(CC) $(KBUILD_CPPFLAGS) -E -P -x c -DSPL_TEXT_BASE=$(CONFIG_SPL_BIN_TEXT_BASE) \
		-DSPL_MAX_SIZE=$(CONFIG_SPL_BIN_MAX_SIZE) $< -o $@
endif

link_flags = $(KBUILD_CFLAGS) -nostdlib -Wl,--gc-sections \
	-Wl,-z,noexecstack -Wl,--whole-archive $(common_builtins) $(1) $(board_libs) \
	-Wl,--no-whole-archive -lgcc

define app_template
app_$(1)_dir := $(image_root)/$(1)
app_$(1)_rel_dir := $(image_rel_root)/$(1)
app_$(1)_builtin := $(objtree)/.obj/apps/$(board)/$(1)/built-in.o
app_$(1)_fel_elf := $$(app_$(1)_dir)/$(1)_fel.elf
app_$(1)_bin_elf := $$(app_$(1)_dir)/$(1)_bin.elf
app_$(1)_fel_bin := $$(app_$(1)_dir)/$(1)_fel.bin
app_$(1)_card_bin := $$(app_$(1)_dir)/$(1)_card.bin
app_$(1)_spi_bin := $$(app_$(1)_dir)/$(1)_spi.bin
app_$(1)_fel_pass1 := $$(app_$(1)_dir)/.$(1)_fel.pass1.elf
app_$(1)_bin_pass1 := $$(app_$(1)_dir)/.$(1)_bin.pass1.elf
app_$(1)_fel_nm := $$(app_$(1)_dir)/.$(1)_fel.symbols.nm
app_$(1)_bin_nm := $$(app_$(1)_dir)/.$(1)_bin.symbols.nm
app_$(1)_fel_symbols_s := $$(app_$(1)_dir)/.$(1)_fel.symbols.S
app_$(1)_bin_symbols_s := $$(app_$(1)_dir)/.$(1)_bin.symbols.S
app_$(1)_fel_symbols_o := $$(app_$(1)_dir)/.$(1)_fel.symbols.o
app_$(1)_bin_symbols_o := $$(app_$(1)_dir)/.$(1)_bin.symbols.o

.PHONY: $(1)
$(1): $$(app_$(1)_fel_bin) $$(app_$(1)_card_bin) $$(app_$(1)_spi_bin)
ifneq ($(filter y,$(CONFIG_BUILD_DEBUG) $(CONFIG_BUILD_TRACE)),)
	@echo "  OBJDUMP $$(app_$(1)_rel_dir)/$(1)_fel.asm"
	@$$(OBJDUMP) -D $$(app_$(1)_fel_elf) > $$(app_$(1)_fel_elf:.elf=.asm)
	@echo "  OBJDUMP $$(app_$(1)_rel_dir)/$(1)_bin.asm"
	@$$(OBJDUMP) -D $$(app_$(1)_bin_elf) > $$(app_$(1)_bin_elf:.elf=.asm)
endif
	@echo "Images are in $$(app_$(1)_rel_dir)"

ifeq ($(CONFIG_BACKTRACE_FULL),y)
$$(app_$(1)_fel_pass1): $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(fel_lds)
	@mkdir -p $$(dir $$@)
	@$$(CC) $$(call link_flags,$$(app_$(1)_builtin)) -T$(fel_lds) -o $$@

$$(app_$(1)_bin_pass1): $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(bin_lds)
	@mkdir -p $$(dir $$@)
	@$$(CC) $$(call link_flags,$$(app_$(1)_builtin)) -T$(bin_lds) -o $$@

$$(app_$(1)_fel_nm): $$(app_$(1)_fel_pass1)
	@$$(NM) -n -S --defined-only $$< > $$@

$$(app_$(1)_bin_nm): $$(app_$(1)_bin_pass1)
	@$$(NM) -n -S --defined-only $$< > $$@

$$(app_$(1)_fel_symbols_s): $$(app_$(1)_fel_nm) $(objtree)/tools/mkbacktrace
	@$(objtree)/tools/mkbacktrace $(backtrace_address_bits) $$< $$@

$$(app_$(1)_bin_symbols_s): $$(app_$(1)_bin_nm) $(objtree)/tools/mkbacktrace
	@$(objtree)/tools/mkbacktrace $(backtrace_address_bits) $$< $$@

$$(app_$(1)_fel_symbols_o): $$(app_$(1)_fel_symbols_s)
	@$$(CC) $$(KBUILD_CPPFLAGS) $$(KBUILD_AFLAGS) -c $$< -o $$@

$$(app_$(1)_bin_symbols_o): $$(app_$(1)_bin_symbols_s)
	@$$(CC) $$(KBUILD_CPPFLAGS) $$(KBUILD_AFLAGS) -c $$< -o $$@

$$(app_$(1)_fel_elf): $$(app_$(1)_fel_symbols_o) $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(fel_lds)
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$(app_$(1)_rel_dir)/$(1)_fel.elf"
	@$$(CC) $$(app_$(1)_fel_symbols_o) $$(call link_flags,$$(app_$(1)_builtin)) -T$(fel_lds) \
		-Wl,-Map,$$(@:.elf=.map) -o $$@
	@cd $(objtree) && $$(SIZE) -B -x $$(app_$(1)_rel_dir)/$(1)_fel.elf

$$(app_$(1)_bin_elf): $$(app_$(1)_bin_symbols_o) $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(bin_lds)
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$(app_$(1)_rel_dir)/$(1)_bin.elf"
	@$$(CC) $$(app_$(1)_bin_symbols_o) $$(call link_flags,$$(app_$(1)_builtin)) -T$(bin_lds) \
		-Wl,-Map,$$(@:.elf=.map) -o $$@
	@cd $(objtree) && $$(SIZE) -B -x $$(app_$(1)_rel_dir)/$(1)_bin.elf
else
$$(app_$(1)_fel_elf): $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(fel_lds)
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$(app_$(1)_rel_dir)/$(1)_fel.elf"
	@$$(CC) $$(call link_flags,$$(app_$(1)_builtin)) -T$(fel_lds) \
		-Wl,-Map,$$(@:.elf=.map) -o $$@
	@cd $(objtree) && $$(SIZE) -B -x $$(app_$(1)_rel_dir)/$(1)_fel.elf

$$(app_$(1)_bin_elf): $(common_builtins) $$(app_$(1)_builtin) $(board_libs) $(bin_lds)
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$(app_$(1)_rel_dir)/$(1)_bin.elf"
	@$$(CC) $$(call link_flags,$$(app_$(1)_builtin)) -T$(bin_lds) \
		-Wl,-Map,$$(@:.elf=.map) -o $$@
	@cd $(objtree) && $$(SIZE) -B -x $$(app_$(1)_rel_dir)/$(1)_bin.elf
endif

$$(app_$(1)_fel_bin): $$(app_$(1)_fel_elf)
	@echo "  OBJCOPY $$(app_$(1)_rel_dir)/$(1)_fel.bin"
	@$$(OBJCOPY) -O binary $$< $$@

$$(app_$(1)_card_bin): $$(app_$(1)_bin_elf) $(objtree)/tools/mksunxi
	@echo "  OBJCOPY $$(app_$(1)_rel_dir)/$(1)_card.bin"
	@$$(OBJCOPY) -O binary $$< $$@
	@$(objtree)/tools/mksunxi $$@ 512

$$(app_$(1)_spi_bin): $$(app_$(1)_bin_elf) $(objtree)/tools/mksunxi
	@echo "  OBJCOPY $$(app_$(1)_rel_dir)/$(1)_spi.bin"
	@$$(OBJCOPY) -O binary $$< $$@
	@$(objtree)/tools/mksunxi $$@ 8192
endef

$(foreach app,$(apps),$(eval $(call app_template,$(app))))

host_tools := mksunxi bin2array bin2asm mkbacktrace
tools: $(addprefix $(objtree)/tools/,$(host_tools)) dt2c-check

utilities:
	@set -e; for utility_dir in $(utility_dirs); do \
		$(MAKE) -C $(srctree)/utils/$$utility_dir \
			BUILD=$(utility_root)/$$utility_dir \
			CROSS_COMPILE=$(BL33_CROSS_COMPILE); \
	done
	@echo "Utility images are in $(utility_rel_root)"

firmware_root := $(objtree)/firmware/$(board)
firmware_rel_root := firmware/$(board)
firmware:
	@if test -z "$(firmware-y)"; then \
		echo "No companion firmware is defined for $(board)"; \
	else \
		set -e; for firmware_dir in $(firmware-y); do \
			$(MAKE) -C $(srctree)/boards/$(board)/$$firmware_dir \
				BUILD=$(firmware_root)/$$firmware_dir \
				CROSS_COMPILE=$(FIRMWARE_CROSS_COMPILE); \
		done; \
		echo "Firmware images are in $(firmware_rel_root)"; \
	fi

check:
	@$(srctree)/utils/check-build-structure.sh

test: dt2c-check
	@$(MAKE) --no-print-directory -C $(srctree)/test

docs:
	@command -v $(DOXYGEN) >/dev/null 2>&1 || { \
		echo "doxygen is required to build the API documentation" >&2; \
		exit 1; \
	}
	@$(DOXYGEN) $(srctree)/Doxyfile
	@echo "API documentation is in docs/api/html"

$(objtree)/tools/%: $(srctree)/tools/%.c
	@mkdir -p $(dir $@)
	@echo "  HOSTCC  tools/$(notdir $<)"
	@$(HOSTCC) -O2 -std=gnu99 $< -o $@

images: tools $(apps)
	@echo "All images are in $(image_rel_root)"

image: images

artifacts: images firmware utilities

help:
	@echo 'Configuration:'
	@echo '  <board>_defconfig       Select a board and its drivers'
	@echo '  menuconfig              Configure with the source-built Kconfig UI'
	@echo '  savedefconfig           Save a minimal configuration'
	@echo '  list-defconfigs         List available board configurations'
	@echo 'Build:'
	@echo '  all                     Build every application for the selected board'
	@echo '  artifacts               Build images, companion firmware, and utilities'
	@echo '  <app>                   Build one application and its three images'
	@echo '  list-apps               List applications for the selected board'
	@echo '  tools                   Build host tools'
	@echo '  utilities               Build standalone BL33 utility images'
	@echo '  firmware                Build companion firmware for the selected board'
	@echo '  check                   Validate the Make/Kconfig tree structure'
	@echo '  test                    Run host and QEMU API tests'
	@echo '  docs                    Build strict Doxygen API documentation'
	@echo '  clean                   Remove generated build files'
	@echo '  mrproper                Also remove configuration files'

clean:
	@$(RM) -r $(objtree)/.obj $(objtree)/build $(objtree)/firmware \
		$(objtree)/utilities $(srctree)/docs/api \
		$(objtree)/tools/mksunxi \
		$(objtree)/tools/bin2array $(objtree)/tools/bin2asm \
		$(objtree)/tools/mkbacktrace \
		$(objtree)/include/generated/config.h

mrproper: clean
	@$(RM) -r $(objtree)/include/config $(objtree)/include/generated \
		$(kconfig_out) $(KCONFIG_CONFIG) $(objtree)/defconfig

FORCE:
