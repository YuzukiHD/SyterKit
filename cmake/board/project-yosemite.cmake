# SPDX-License-Identifier: GPL-2.0+

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_CHIP_SUN8IW21 True)
set(CONFIG_BOARD_PROJECT_YOSEMITE True)

add_definitions(-DCONFIG_CHIP_SUN8IW21)

# Options

# By setting ENABLE_HARDFP to ON, it indicates that the project is configured
# to utilize hard floating-point operations when applicable. This can be beneficial 
# in scenarios where performance gains from hardware acceleration are desired.
option(ENABLE_HARDFP "Enable hardware floating-point operations" ON)

# Set the cross-compile toolchain
set(CROSS_COMPILE "arm-none-eabi-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

# Configure compiler flags based on ENABLE_HARDFP option
if(ENABLE_HARDFP)
    set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -g -ggdb -O3 -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard")
else()
    set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -g -ggdb -O3 -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=softfp")
endif()

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(ARCH_BIN_START_ADDRESS "0x00020000")
set(ARCH_BIN_SRAM_LENGTH "128K")

set(ARCH_FEL_START_ADDRESS "0x00028000")
set(ARCH_FEL_SRAM_LENGTH "100K")