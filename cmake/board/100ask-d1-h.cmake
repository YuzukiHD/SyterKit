# SPDX-License-Identifier: GPL-2.0+

set(CONFIG_ARCH_RISCV64 True)
set(CONFIG_ARCH_RISCV64_CORE_C906 True)
set(CONFIG_CHIP_SUN20IW1 True)
set(CONFIG_BOARD_100ASK_D1_H True)

add_definitions(-DCONFIG_CHIP_SUN20IW1)

# Set the cross-compile toolchain
if(DEFINED ENV{RISCV_ROOT_PATH})
    file(TO_CMAKE_PATH $ENV{RISCV_ROOT_PATH} RISCV_ROOT_PATH)
else()
    message(FATAL_ERROR "RISCV_ROOT_PATH env must be defined")
endif()

set(RISCV_ROOT_PATH ${RISCV_ROOT_PATH} CACHE STRING "root path to riscv toolchain")

set(CROSS_COMPILE "${RISCV_ROOT_PATH}/riscv64-unknown-elf-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

# Configure compiler flags based on ENABLE_HARDFP option
set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -Os -march=rv64gcv0p7_zfh_xtheadc -mabi=lp64d -mtune=c906 -mcmodel=medlow -fno-stack-protector -mstrict-align")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-builtin-declaration-mismatch -Wno-pointer-to-int-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-builtin-declaration-mismatch")

set(ARCH_BIN_START_ADDRESS "0x00020000")
set(ARCH_BIN_SRAM_LENGTH "128K")

set(ARCH_FEL_START_ADDRESS "0x00028000")
set(ARCH_FEL_SRAM_LENGTH "128K")
