# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := host
TEST_OUT ?= ../../out

.PHONY: all run
all: run
run:
	@bash $(CURDIR)/verify.sh $(abspath $(TEST_OUT))/defconfig_list
