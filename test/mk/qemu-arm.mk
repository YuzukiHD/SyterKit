# SPDX-License-Identifier: GPL-2.0+

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
CASE_NAME := $(notdir $(CURDIR))
TEST_OUT ?= $(TEST_ROOT)/out
BUILD_DIR := $(TEST_OUT)/$(CASE_NAME)
TEST_ELF := $(BUILD_DIR)/$(CASE_NAME).elf
TEST_LOG := $(BUILD_DIR)/actual.txt
ARM_CROSS_COMPILE ?= arm-none-eabi-
CC := $(ARM_CROSS_COMPILE)gcc
NM := $(ARM_CROSS_COMPILE)nm
ADDR2LINE := $(ARM_CROSS_COMPILE)addr2line
QEMU_ARM ?= qemu-arm
QEMU_TIMEOUT ?= 10

TEST_OPT ?= -O0
TEST_BACKTRACE_MODE ?= min
TEST_EXTRA_CPPFLAGS ?=
TEST_EXTRA_CFLAGS ?=
TEST_EXTRA_SOURCES ?=
ifeq ($(TEST_BACKTRACE_MODE),full)
TEST_BACKTRACE_CPPFLAGS := -DCONFIG_BACKTRACE=1 -DCONFIG_BACKTRACE_FULL=1
TEST_BACKTRACE_SOURCE := $(srctree)/core/backtrace/backtrace-full.c
else
TEST_BACKTRACE_CPPFLAGS := -DCONFIG_BACKTRACE=1 -DCONFIG_BACKTRACE_MIN=1
TEST_BACKTRACE_SOURCE := $(srctree)/core/backtrace/backtrace-min.c
endif
TEST_CPPFLAGS := -I$(srctree)/arch/arm/include -I$(TEST_ROOT)/include \
	-I$(srctree)/include -DCONFIG_ARCH_ARM32=1 -DXCFG_FORMAT_FLOAT=0 \
	$(TEST_BACKTRACE_CPPFLAGS) $(TEST_EXTRA_CPPFLAGS)
TEST_CFLAGS := -std=gnu11 $(TEST_OPT) -g -Wall -Wextra -Werror -ffreestanding \
	-fno-builtin -fno-common -fno-stack-protector -fno-omit-frame-pointer \
	-fno-optimize-sibling-calls -ffunction-sections -fdata-sections \
	-mcpu=cortex-a7 -mthumb -mthumb-interwork -mfloat-abi=softfp \
	-mfpu=neon-vfpv4 $(TEST_EXTRA_CFLAGS)
TEST_AFLAGS := $(TEST_CFLAGS)
TEST_LDFLAGS := -nostdlib -nostartfiles -Wl,--gc-sections \
	-Wl,--build-id=none -Wl,-z,noexecstack -Wl,-T,$(CURDIR)/linker.ld
all_sources := $(TEST_ROOT)/support/qemu/arm/start.S \
	$(TEST_ROOT)/support/qemu/arm/runtime.c $(CURDIR)/main.c \
	$(srctree)/arch/arm/backtrace.c $(TEST_BACKTRACE_SOURCE) \
	$(TEST_EXTRA_SOURCES)

ifeq ($(TEST_BACKTRACE_MODE),full)
include $(TEST_ROOT)/mk/backtrace-symbols.mk
endif

.PHONY: all run clean

all: $(TEST_ELF)

$(TEST_ELF): $(all_sources) $(CURDIR)/linker.ld $(BACKTRACE_LINK_DEPS)
	@mkdir -p $(BUILD_DIR)
	@echo "  ARMCC   test/$(CASE_NAME)"
	@$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) $(all_sources) \
		$(BACKTRACE_LINK_INPUTS) \
		$(TEST_LDFLAGS) -lgcc -o $@

run: $(TEST_ELF)
	@command -v $(QEMU_ARM) >/dev/null || { \
		echo "test: $(QEMU_ARM) is required" >&2; exit 1; }
	@echo "  QEMU    $(CASE_NAME)"
	@set +e; timeout $(QEMU_TIMEOUT) $(QEMU_ARM) -cpu cortex-a7 \
		$(TEST_ELF) >$(TEST_LOG) 2>&1; status=$$?; \
	set -e; if [ $$status -ne 0 ]; then cat $(TEST_LOG); exit $$status; fi
	@$(CURDIR)/verify.sh $(CURDIR)/data $(TEST_LOG) $(TEST_ELF) \
		$(NM) $(ADDR2LINE)
	@cat $(TEST_LOG)

clean:
	@$(RM) -r $(BUILD_DIR)
