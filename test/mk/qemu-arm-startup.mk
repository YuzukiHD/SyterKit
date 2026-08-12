# SPDX-License-Identifier: GPL-2.0+

TEST_TYPE := qemu-arm-system
TEST_ROOT := $(abspath $(CURDIR)/../..)
srctree := $(abspath $(TEST_ROOT)/..)
CASE_NAME := $(notdir $(CURDIR))
TEST_OUT ?= $(TEST_ROOT)/out
BUILD_DIR := $(TEST_OUT)/$(CASE_NAME)
TEST_ELF := $(BUILD_DIR)/$(CASE_NAME).elf
TEST_LDS := $(BUILD_DIR)/syterkit.lds
TEST_LOG := $(BUILD_DIR)/actual.txt
ARM_CROSS_COMPILE ?= arm-none-eabi-
CC := $(ARM_CROSS_COMPILE)gcc
NM := $(ARM_CROSS_COMPILE)nm
OBJDUMP := $(ARM_CROSS_COMPILE)objdump
OBJCOPY := $(ARM_CROSS_COMPILE)objcopy
QEMU_TIMEOUT ?= 2
QEMU_MACHINE ?= virt
TEST_MINSTACK ?= 0
TEST_AARCH64_TRANSITION ?= 0
TEST_LOAD_ADDRESS := 0x40000000

TEST_CPPFLAGS := -I$(srctree)/arch/arm/include -I$(srctree)/include \
	-DCONFIG_ARCH_ARM32=1 $(TEST_EXTRA_CPPFLAGS)
TEST_CFLAGS := -std=gnu11 -O2 -g -Wall -Wextra -Werror -ffreestanding \
	-fno-builtin -fno-common -fno-stack-protector -ffunction-sections \
	-fdata-sections -mthumb -mthumb-interwork -mfloat-abi=softfp \
	-mfpu=neon $(TEST_ARCH_FLAGS)
TEST_AFLAGS := $(TEST_CFLAGS)
TEST_LDFLAGS := -nostdlib -nostartfiles -Wl,--gc-sections \
	-Wl,--build-id=none -Wl,-z,noexecstack -Wl,-e,test_entry \
	-Wl,-T,$(TEST_LDS)
TEST_SOURCES := $(TEST_ROOT)/support/qemu/arm-startup/start.S \
	$(srctree)/arch/arm/start.S \
	$(TEST_ROOT)/support/qemu/arm-startup/runtime.c \
	$(srctree)/core/initcall.c

ifeq ($(TEST_AARCH64_TRANSITION),1)
TEST_LOAD_ADDRESS := 0x41000000
AARCH64_CROSS_COMPILE ?= aarch64-linux-gnu-
AARCH64_AS := $(AARCH64_CROSS_COMPILE)as
AARCH64_LD := $(AARCH64_CROSS_COMPILE)ld
AARCH64_OBJCOPY := $(AARCH64_CROSS_COMPILE)objcopy
TEST_BIN := $(BUILD_DIR)/$(CASE_NAME).bin
TRANSITION_OBJ := $(BUILD_DIR)/aarch64-transition.o
TRANSITION_ELF := $(BUILD_DIR)/aarch64-transition.elf
TRANSITION_BIN := $(BUILD_DIR)/aarch64-transition.bin
QEMU_DEPS := $(TEST_BIN) $(TRANSITION_BIN)
QEMU_BOOT_ARGS := -M $(QEMU_MACHINE),secure=on -bios $(TRANSITION_BIN) \
	-device loader,file=$(TEST_BIN),addr=$(TEST_LOAD_ADDRESS),force-raw=on
else
QEMU_DEPS := $(TEST_ELF)
QEMU_BOOT_ARGS := -M $(QEMU_MACHINE) -kernel $(TEST_ELF)
endif

.PHONY: all run clean

all: $(TEST_ELF)

$(TEST_LDS): $(srctree)/arch/arm/syterkit.lds.S
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(TEST_CPPFLAGS) -DSPL_TEXT_BASE=$(TEST_LOAD_ADDRESS) \
		-DSPL_MAX_SIZE=0x01000000 -E -P -x assembler-with-cpp \
		$< -o $@

$(TEST_ELF): $(TEST_SOURCES) $(TEST_LDS)
	@mkdir -p $(BUILD_DIR)
	@echo "  ARMCC   test/$(CASE_NAME)"
	@$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) $(TEST_SOURCES) \
		$(TEST_LDFLAGS) -o $@

ifeq ($(TEST_AARCH64_TRANSITION),1)
$(TEST_BIN): $(TEST_ELF)
	@$(OBJCOPY) -O binary $< $@

$(TRANSITION_OBJ): $(TEST_ROOT)/support/qemu/arm-startup/aarch64-transition.S
	@mkdir -p $(BUILD_DIR)
	@$(AARCH64_AS) $< -o $@

$(TRANSITION_BIN): $(TRANSITION_OBJ) $(TEST_ELF) \
		$(TEST_ROOT)/support/qemu/arm-startup/aarch64-transition.ld
	@entry=`$(NM) -n $(TEST_ELF) | awk '$$3 == "test_entry" { print "0x" $$1 }'`; \
		test -n "$$entry"; \
		$(AARCH64_LD) -T \
			$(TEST_ROOT)/support/qemu/arm-startup/aarch64-transition.ld \
			--defsym=AARCH32_ENTRY=$$entry $(TRANSITION_OBJ) \
			-o $(TRANSITION_ELF)
	@$(AARCH64_OBJCOPY) -O binary $(TRANSITION_ELF) $@
endif

run: $(QEMU_DEPS)
	@command -v $(QEMU_SYSTEM) >/dev/null || { \
		echo "test: $(QEMU_SYSTEM) is required" >&2; exit 1; }
	@echo "  QEMU    $(CASE_NAME)"
	@set +e; timeout $(QEMU_TIMEOUT) $(QEMU_SYSTEM) \
		$(QEMU_BOOT_ARGS) -cpu $(QEMU_CPU) -nographic \
		-monitor none -serial none -nic none -no-reboot \
		-semihosting-config enable=on,target=native \
		>$(TEST_LOG) 2>&1; status=$$?; \
	set -e; if [ $$status -ne 124 ]; then \
		cat $(TEST_LOG); \
		echo "test: expected QEMU to remain in the post-main WFI loop" >&2; \
		exit 1; \
	fi
	@$(TEST_ROOT)/support/qemu/arm-startup/verify.sh \
		$(TEST_LOG) $(TEST_ELF) $(NM) $(OBJDUMP) $(TEST_MINSTACK)
	@sed '/qemu-system-.*terminating on signal/d' $(TEST_LOG)

clean:
	@$(RM) -r $(BUILD_DIR)
