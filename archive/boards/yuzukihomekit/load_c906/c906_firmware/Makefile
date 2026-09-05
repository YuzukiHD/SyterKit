# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
BUILD ?= build

CFLAGS := -nostdlib -nostdinc -Os -mcpu=c906fdv -mcmodel=medany \
	-fno-stack-protector -ffreestanding -Wall -Iinclude -I$(BUILD)
LDFLAGS := -nostdlib -T link.ld -Wl,--gc-sections
sources := src/start.S src/main.c src/uart.c drivers/clock.c \
	drivers/serial.c lib/memcpy.S lib/memset.S
objects := $(addprefix $(BUILD)/,$(addsuffix .o,$(basename $(sources))))

.PHONY: all clean
all: $(BUILD)/c906.bin

$(BUILD)/config.h:
	@mkdir -p $(dir $@)
	@{ \
		echo '#ifndef __C906_CONFIG_H__'; \
		echo '#define __C906_CONFIG_H__'; \
		echo '#define PROJECT_NAME "C906 firmware"'; \
		echo '#define PROJECT_GIT_HASH 0'; \
		echo '#define UART0_BASE_ADDR 0x02500c00'; \
		echo '#endif'; \
	} > $@

$(BUILD)/%.o: %.c $(BUILD)/config.h
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.S $(BUILD)/config.h
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/c906.elf: $(objects) link.ld
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(objects) -lgcc -o $@

$(BUILD)/c906.bin: $(BUILD)/c906.elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

clean:
	@$(RM) -r $(BUILD)

-include $(objects:.o=.d)
