# SPDX-License-Identifier: GPL-2.0+

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
CASE_NAME := $(notdir $(CURDIR))
TEST_OUT ?= $(TEST_ROOT)/out
BUILD_DIR := $(TEST_OUT)/$(CASE_NAME)
TEST_BINARY := $(BUILD_DIR)/$(CASE_NAME)
TEST_LOG := $(BUILD_DIR)/actual.txt
HOSTCC ?= cc

HOST_CPPFLAGS := -I$(TEST_ROOT)/include -I$(srctree)/include \
	-DTEST_CASE_NAME=\"$(CASE_NAME)\" $(TEST_CPPFLAGS)
HOST_CFLAGS := -std=gnu11 -O0 -g -Wall -Werror \
	-fno-builtin $(TEST_CFLAGS)
all_sources := $(CURDIR)/main.c $(TEST_SRCS) $(TEST_ROOT)/support/host.c

.PHONY: all run clean

all: $(TEST_BINARY)

$(TEST_BINARY): $(all_sources)
	@mkdir -p $(BUILD_DIR)
	@echo "  HOSTCC  test/$(CASE_NAME)"
	@$(HOSTCC) $(HOST_CPPFLAGS) $(HOST_CFLAGS) $(all_sources) \
		$(TEST_LDFLAGS) $(TEST_LDLIBS) -o $@

run: $(TEST_BINARY)
	@echo "  TEST    $(CASE_NAME)"
	@$(TEST_BINARY) $(CURDIR) >$(TEST_LOG) 2>&1
	@$(CURDIR)/verify.sh $(CURDIR)/data/expected.txt $(TEST_LOG)
	@cat $(TEST_LOG)

clean:
	@$(RM) -r $(BUILD_DIR)
