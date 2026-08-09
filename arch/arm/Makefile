# SPDX-License-Identifier: GPL-2.0+

CROSS_COMPILE ?= arm-none-eabi-

KBUILD_CFLAGS += -fno-short-enums

ifeq ($(CONFIG_CPU_CORTEX_A7),y)
KBUILD_CFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb \
	-mno-unaligned-access -mfpu=neon-vfpv4
KBUILD_AFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb \
	-mno-unaligned-access -mfpu=neon-vfpv4
ifeq ($(CONFIG_ARM_HARDFP),y)
KBUILD_CFLAGS += -mfloat-abi=hard
KBUILD_AFLAGS += -mfloat-abi=hard
else
KBUILD_CFLAGS += -mfloat-abi=softfp
KBUILD_AFLAGS += -mfloat-abi=softfp
endif
endif

ifeq ($(CONFIG_CPU_ARMV8),y)
KBUILD_CFLAGS += -march=armv8-a -mthumb-interwork -mthumb \
	-mfpu=neon -mfloat-abi=softfp
KBUILD_AFLAGS += -march=armv8-a -mthumb-interwork -mthumb \
	-mfpu=neon -mfloat-abi=softfp
endif

ifeq ($(CONFIG_CPU_ARMV8_2),y)
KBUILD_CFLAGS += -march=armv8.2-a -mthumb-interwork -mthumb \
	-mfpu=neon -mfloat-abi=softfp
KBUILD_AFLAGS += -march=armv8.2-a -mthumb-interwork -mthumb \
	-mfpu=neon -mfloat-abi=softfp
endif
