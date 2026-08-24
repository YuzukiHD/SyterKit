# SPDX-License-Identifier: GPL-2.0+

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
CASE_NAME := $(notdir $(CURDIR))
TEST_OUT ?= $(TEST_ROOT)/out
BUILD_DIR := $(TEST_OUT)/$(CASE_NAME)
TEST_ELF := $(BUILD_DIR)/$(CASE_NAME).elf
TEST_LOG := $(BUILD_DIR)/actual.txt
RISCV_CROSS_COMPILE ?= riscv64-unknown-elf-
CC := $(RISCV_CROSS_COMPILE)gcc
NM := $(RISCV_CROSS_COMPILE)nm
ADDR2LINE := $(RISCV_CROSS_COMPILE)addr2line
QEMU_RISCV32 ?= qemu-riscv32
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
TEST_CPPFLAGS := -I$(srctree)/arch/riscv/include -I$(TEST_ROOT)/include \
	-I$(srctree)/include -DCONFIG_ARCH_RISCV32=1 \
	$(TEST_BACKTRACE_CPPFLAGS) $(TEST_EXTRA_CPPFLAGS)
TEST_CFLAGS := -std=gnu11 $(TEST_OPT) -g -Wall -Wextra -Werror -ffreestanding \
	-fno-builtin -fno-common -fno-stack-protector -fno-omit-frame-pointer \
	-fno-optimize-sibling-calls -ffunction-sections -fdata-sections \
	-march=rv32imafc_zicsr -mabi=ilp32f -msmall-data-limit=0 \
	$(TEST_EXTRA_CFLAGS)
TEST_AFLAGS := $(TEST_CFLAGS)
TEST_LDFLAGS := -nostdlib -nostartfiles -Wl,--gc-sections \
	-Wl,--build-id=none -Wl,-z,noexecstack -Wl,-melf32lriscv \
	-Wl,-T,$(CURDIR)/linker.ld
all_sources := $(TEST_ROOT)/support/qemu/riscv/start.S \
	$(TEST_ROOT)/support/qemu/riscv/runtime.c $(CURDIR)/main.c \
	$(srctree)/arch/riscv/backtrace-rv32.c \
	$(TEST_BACKTRACE_SOURCE) $(TEST_EXTRA_SOURCES)

ifeq ($(TEST_BACKTRACE_MODE),full)
include $(TEST_ROOT)/mk/backtrace-symbols.mk
endif

.PHONY: all run clean

all: $(TEST_ELF)

$(TEST_ELF): $(all_sources) $(CURDIR)/linker.ld $(BACKTRACE_LINK_DEPS)
	@mkdir -p $(BUILD_DIR)
	@echo "  E907CC  test/$(CASE_NAME)"
	@$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) $(all_sources) \
		$(BACKTRACE_LINK_INPUTS) \
		$(TEST_LDFLAGS) -lgcc -o $@
ifeq ($(TEST_BACKTRACE_MODE),full)
	@$(NM) -n -S --defined-only $@ > $(BACKTRACE_NM)
	@$(BACKTRACE_TOOL) $(TEST_BACKTRACE_BITS) $(BACKTRACE_NM) $(BACKTRACE_SYMBOLS_S)
	@$(CC) $(TEST_CPPFLAGS) $(TEST_AFLAGS) -c $(BACKTRACE_SYMBOLS_S) -o $(BACKTRACE_SYMBOLS_O)
	@$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) $(all_sources) \
		$(BACKTRACE_LINK_INPUTS) \
		$(TEST_LDFLAGS) -lgcc -o $@
endif

run: $(TEST_ELF)
	@command -v $(QEMU_RISCV32) >/dev/null || { \
		echo "test: $(QEMU_RISCV32) is required" >&2; exit 1; }
	@echo "  QEMU    $(CASE_NAME)"
	@set +e; timeout $(QEMU_TIMEOUT) $(QEMU_RISCV32) \
		-cpu rv32 $(TEST_ELF) >$(TEST_LOG) 2>&1; status=$$?; \
	set -e; if [ $$status -ne 0 ]; then cat $(TEST_LOG); exit $$status; fi
	@$(CURDIR)/verify.sh $(CURDIR)/data $(TEST_LOG) $(TEST_ELF) \
		$(NM) $(ADDR2LINE)
	@cat $(TEST_LOG)

clean:
	@$(RM) -r $(BUILD_DIR)
