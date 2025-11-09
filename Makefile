# SyterKit Build System
# SPDX-License-Identifier: GPL-2.0+

# Set the default target
.DEFAULT_GOAL := all

# Set the RISCV compiler path
RISCV_ROOT_PATH ?= /home/yuzuki/sdk/Xuantie-900-gcc-elf-newlib-x86_64-V3.2.0/bin/

# Build DIR
BUILD_DIR_OUT := build

# Get list of board targets (without .cmake extension) and define build directories
BOARD_TARGETS:=$(shell find ./cmake/board/ -name "*.cmake" -type f -exec basename -s .cmake {} \;)
BUILD_DIRS:=$(addprefix $(BUILD_DIR_OUT)/,$(BOARD_TARGETS))

# Rule to build all targets
all: $(BUILD_DIRS)

# Rule to build each board configuration
$(BUILD_DIRS): $(BUILD_DIR_OUT)/%: cmake/board/%.cmake
	@echo "Building for board: $*"
	@echo "Build directory: $@"
	@echo "RISCV_ROOT_PATH: $(RISCV_ROOT_PATH)"
	@echo "CMAKE_BOARD_FILE: $*.cmake"
	@echo "CMAKE_BOARD_FILE_PATH: $(CMAKE_BOARD_FILE_PATH)"
	@echo "clean build directory..."
	@if [ -d $(BUILD_DIR_OUT) ]; then \
		if [ -d $@ ]; then \
			rm -rf $@; \
		fi; \
	fi
	@echo "create build directory..."
	@mkdir -p $@
	@echo "run cmake..."
	@cd $@ && \
	export RISCV_ROOT_PATH=$(RISCV_ROOT_PATH) && \
	cmake -B . -DCMAKE_BUILD_TYPE=Release -DCMAKE_BOARD_FILE=$*.cmake ../.. && \
	cmake --build . --config Release -j

# Clean all build directories
clean:
	@echo "Cleaning all build directories..."
	@rm -rf $(BUILD_DIRS)

# Help target
help:
	@echo "SyterKit Build System"
	@echo "Usage: make [TARGET]"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build all board configurations (default)"
	@echo "  clean     - Remove all build directories"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Board configurations:"
	@for target in $(BOARD_TARGETS); do \
	  echo "  $(BUILD_DIR_OUT)/$$target - Build for $$target"; \
	done

.PHONY: all clean help