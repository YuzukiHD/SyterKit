# SPDX-License-Identifier: GPL-2.0+

TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
CASE_NAME := $(notdir $(CURDIR))
TEST_OUT ?= $(TEST_ROOT)/out
RUST_TEST_TARGET_DIR := $(TEST_OUT)/$(CASE_NAME)/cargo
RUST_CARGO ?= cargo

RUST_TEST_CLANG_ARGS := --target=x86_64-unknown-linux-gnu \
	-DCONFIG_DRIVER_MMC_TUNING=1 \
	-DCONFIG_DRIVER_GPIO_V2_POW=1 \
	-I$(srctree)/arch/arm/include \
	-I$(srctree)/include \
	-I$(srctree)/dts/include

.PHONY: all run clean

all: run

run:
	@echo "  CARGO   test/$(CASE_NAME)"
	@SYTERKIT_SOURCE_DIR="$(srctree)" \
	 SYTERKIT_FFI_HEADER_MANIFEST="$(srctree)/rust/ffi/headers.txt" \
	 SYTERKIT_FFI_ARCH=arm \
	 SYTERKIT_FFI_CLANG_ARGS="$(RUST_TEST_CLANG_ARGS)" \
	 CARGO_TARGET_DIR="$(RUST_TEST_TARGET_DIR)" \
	 $(RUST_CARGO) test --locked --manifest-path "$(srctree)/Cargo.toml" \
		--package syterkit-ffi --package syterkit-drivers \
		--package syterkit-core --package syterkit-lib

clean:
	@$(RM) -r "$(TEST_OUT)/$(CASE_NAME)"
