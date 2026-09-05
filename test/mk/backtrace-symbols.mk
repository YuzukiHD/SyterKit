# SPDX-License-Identifier: GPL-2.0+

HOSTCC ?= cc
BACKTRACE_TOOL := $(TEST_OUT)/tools/mkbacktrace
BACKTRACE_PASS1_ELF := $(BUILD_DIR)/.$(CASE_NAME).pass1.elf
BACKTRACE_NM := $(BUILD_DIR)/.$(CASE_NAME).symbols.nm
BACKTRACE_SYMBOLS_S := $(BUILD_DIR)/.$(CASE_NAME).symbols.S
BACKTRACE_SYMBOLS_O := $(BUILD_DIR)/.$(CASE_NAME).symbols.o
BACKTRACE_LINK_DEPS := $(BACKTRACE_SYMBOLS_O)
BACKTRACE_LINK_INPUTS := $(BACKTRACE_SYMBOLS_O)
TEST_BACKTRACE_BITS ?= 32

$(BACKTRACE_TOOL): $(srctree)/tools/mkbacktrace.c
	@mkdir -p $(dir $@)
	@echo "  HOSTCC  tools/mkbacktrace.c"
	@$(HOSTCC) -O2 -std=gnu99 $< -o $@

$(BACKTRACE_PASS1_ELF): $(all_sources) $(CURDIR)/linker.ld
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) $(all_sources) \
		$(TEST_LDFLAGS) -lgcc -o $@

$(BACKTRACE_NM): $(BACKTRACE_PASS1_ELF)
	@$(NM) -n -S --defined-only $< > $@

$(BACKTRACE_SYMBOLS_S): $(BACKTRACE_NM) $(BACKTRACE_TOOL)
	@$(BACKTRACE_TOOL) $(TEST_BACKTRACE_BITS) $< $@

$(BACKTRACE_SYMBOLS_O): $(BACKTRACE_SYMBOLS_S)
	@$(CC) $(TEST_CPPFLAGS) $(TEST_AFLAGS) -c $< -o $@
