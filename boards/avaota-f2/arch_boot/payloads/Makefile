# SPDX-License-Identifier: GPL-2.0+

# The helper always executes on E907 as an RV32 image, including when it
# starts a C907 configured for RV64.
MARCH := -mtune=e907 -march=rv32imafc_zicntr_zicsr_zifencei_zihpm_xtheade
MABI := -mabi=ilp32f

candidate_cross := $(if $(CROSS_COMPILE),$(CROSS_COMPILE),) \
	../../../../../Xuantie-900-gcc-elf-newlib-x86_64-V3.2.0/bin/riscv64-unknown-elf- \
	/tmp/xuantie-v3.2.0/bin/riscv64-unknown-elf- \
	riscv64-unknown-elf-

CROSS ?= $(shell \
	for c in $(candidate_cross); do \
		if printf '.text\nnop\n' | "$$c"gcc $(MARCH) $(MABI) \
			-x assembler-with-cpp -c -o /dev/null - 2>/dev/null; then \
			echo "$$c"; exit 0; \
		fi; \
	done; \
	echo riscv64-unknown-elf-)

AS := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

ASFLAGS := -c $(MARCH) $(MABI)
LDFLAGS := -m elf32lriscv --gc-sections

BUILD_DIR := build
RV32_DIR := $(BUILD_DIR)/rv32
RV64_DIR := $(BUILD_DIR)/rv64
RV32_BIN := $(RV32_DIR)/c907_boot0.bin
RV64_BIN := $(RV64_DIR)/c907_boot0.bin

all: ../arch_boot.c

$(RV32_DIR) $(RV64_DIR):
	mkdir -p $@

$(RV32_DIR)/c907_boot0.o: c907_boot0.S | $(RV32_DIR)
	$(AS) $(ASFLAGS) -DC907_CORE_MODE=0x1 $< -o $@

$(RV64_DIR)/c907_boot0.o: c907_boot0.S | $(RV64_DIR)
	$(AS) $(ASFLAGS) -DC907_CORE_MODE=0x2 $< -o $@

$(RV32_DIR)/c907_boot0.elf: $(RV32_DIR)/c907_boot0.o arch_boot.lds
	$(LD) $(LDFLAGS) -T arch_boot.lds $< -o $@

$(RV64_DIR)/c907_boot0.elf: $(RV64_DIR)/c907_boot0.o arch_boot.lds
	$(LD) $(LDFLAGS) -T arch_boot.lds $< -o $@

$(RV32_BIN): $(RV32_DIR)/c907_boot0.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(RV64_BIN): $(RV64_DIR)/c907_boot0.elf
	$(OBJCOPY) -O binary -j .text $< $@

../arch_boot.c: $(RV32_BIN) $(RV64_BIN) gen_payload.py
	python3 gen_payload.py $(RV32_BIN) $(RV64_BIN) $@

clean:
	rm -rf $(BUILD_DIR) rv32 rv64

.PHONY: all clean
