# SPDX-License-Identifier: Apache-2.0

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_ARCH_ARM32_ARM64 True)
set(CONFIG_CHIP_SUN50IW9 True)
set(CONFIG_USE_DRAM_PAYLOAD True)
set(CONFIG_BOARD_LONGANPI-3H True)

add_definitions(-DCONFIG_CHIP_SUN50IW9)

# Set the cross-compile toolchain
set(CROSS_COMPILE "arm-linux-gnueabi-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

set(CMAKE_COMMON_FLAGS "-nostdlib -g -ggdb -O3 -mcpu=cortex-a53")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(ARCH_BIN_START_ADDRESS "0x00020000")
set(ARCH_BIN_SRAM_LENGTH "128K")

set(ARCH_FEL_START_ADDRESS "0x00028000")
set(ARCH_FEL_SRAM_LENGTH "100K")