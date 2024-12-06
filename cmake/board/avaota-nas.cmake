# SPDX-License-Identifier: Apache-2.0

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_ARCH_ARM32_ARM64 True)
set(CONFIG_CHIP_SUN60IW2 True)
set(CONFIG_CHIP_WITHPMU True)
set(CONFIG_CHIP_DCACHE True)
set(CONFIG_CHIP_MMC_V2 True)
set(CONFIG_CHIP_GPIO_V3 True)
set(CONFIG_BOARD_AVAOTA-NAS True)

add_definitions(-DCONFIG_CHIP_SUN60IW2) 
add_definitions(-DCONFIG_CHIP_DCACHE)
add_definitions(-DCONFIG_CHIP_MMC_V2)
add_definitions(-DCONFIG_CHIP_GPIO_V3)
add_definitions(-DCONFIG_FATFS_CACHE_SIZE=0xFFFFFFF)
add_definitions(-DCONFIG_FATFS_CACHE_ADDR=0x60000000)

# Set the cross-compile toolchain
set(CROSS_COMPILE "arm-none-eabi-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

set(CMAKE_COMMON_FLAGS "-nostdlib -nostdinc -O0 -march=armv8.4-a -mthumb-interwork -fno-common -ffunction-sections -fno-builtin -fno-stack-protector -ffreestanding -mthumb -mfpu=neon -mfloat-abi=softfp -pipe")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(ARCH_BIN_START_ADDRESS "0x00047000")
set(ARCH_BIN_SRAM_LENGTH "256K")

set(ARCH_FEL_START_ADDRESS "0x00048c00")
set(ARCH_FEL_SRAM_LENGTH "240K")
