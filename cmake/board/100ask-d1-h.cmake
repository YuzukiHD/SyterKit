# SPDX-License-Identifier: Apache-2.0

set(CONFIG_ARCH_RISCV64 True)
set(CONFIG_CHIP_SUN20IW1 True)
set(CONFIG_CHIP_SUN8IW20 True) # CONFIG_CHIP_SUN20IW1 share same pref driver as CONFIG_CHIP_SUN8IW20
set(CONFIG_BOARD_100ASK_D1_H True)

add_definitions(-DCONFIG_CHIP_SUN8IW20)

# Set the cross-compile toolchain
set(CROSS_COMPILE "/home/yuzuki/sdk/Xuantie-900-gcc-elf-newlib-x86_64-V2.8.1/bin/riscv64-unknown-elf-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

# Configure compiler flags based on ENABLE_HARDFP option
set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d -mtune=c906 -mcmodel=medlow -fno-stack-protector -mstrict-align")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-builtin-declaration-mismatch -Wno-pointer-to-int-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-builtin-declaration-mismatch")

set(ARCH_BIN_START_ADDRESS "0x00020000")
set(ARCH_BIN_SRAM_LENGTH "128K")

set(ARCH_FEL_START_ADDRESS "0x00028000")
set(ARCH_FEL_SRAM_LENGTH "100K")
