# SPDX-License-Identifier: GPL-2.0+

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_CHIP_SUN8IW22 True)
set(CONFIG_CHIP_MMC_V2 True)
set(CONFIG_CHIP_GPIO_V3 True)
set(CONFIG_BOARD_TLT153-MINIEVM True)

add_definitions(-DCONFIG_CHIP_SUN8IW22) 
add_definitions(-DCONFIG_FATFS_CACHE_SIZE=0x2000000) 
add_definitions(-DCONFIG_FATFS_CACHE_ADDR=0x48000000)

# Set the cross-compile toolchain
if(DEFINED ENV{LINARO_GCC_721_PATH})
    file(TO_CMAKE_PATH $ENV{LINARO_GCC_721_PATH} LINARO_GCC_721_PATH)
else()
    message(FATAL_ERROR "LINARO_GCC_721_PATH for gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi env must be defined")
endif()

set(LINARO_GCC_721_PATH ${LINARO_GCC_721_PATH} CACHE STRING "root path to gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi toolchain")

set(CROSS_COMPILE "${LINARO_GCC_721_PATH}/arm-linux-gnueabi-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -g -ggdb -O0 -mcpu=cortex-a7 -mthumb-interwork -fno-common -ffunction-sections -fno-builtin -fno-stack-protector -ffreestanding -mthumb -mfpu=neon -mfloat-abi=softfp -pipe")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(ARCH_BIN_START_ADDRESS "0x00040000")
set(ARCH_BIN_SRAM_LENGTH "160K")

set(ARCH_FEL_START_ADDRESS "0x00048000")
set(ARCH_FEL_SRAM_LENGTH "128K")