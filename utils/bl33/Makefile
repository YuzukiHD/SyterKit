# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
BUILD ?= output

src := $(CURDIR)
name := syter_bl33
sources := source/start.S source/memcpy.S source/memset.S \
	source/smccc-call.S source/main.c source/printf.c source/string.c \
	source/timer.c source/xformat.c
objects := $(addprefix $(BUILD)/,$(addsuffix .o,$(basename $(sources))))
deps := $(objects:.o=.d)

CFLAGS := -nostdlib -nostdinc -ffreestanding -fno-builtin \
	-Wall -O3 -g -ggdb -mabi=aapcs -march=armv7-a -I$(src)/include
LDFLAGS := -nostdlib -T$(src)/link.ld

.PHONY: all clean
all: $(BUILD)/$(name).bin

$(BUILD)/$(name).elf: $(objects) $(src)/link.ld
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(objects) -Wl,-Map,$(@:.elf=.map) \
		-lgcc -o $@

$(BUILD)/$(name).bin: $(BUILD)/$(name).elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

$(BUILD)/%.o: $(src)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: $(src)/%.S
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	@$(RM) -r $(BUILD)

-include $(deps)
