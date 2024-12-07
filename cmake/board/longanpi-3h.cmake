# SPDX-License-Identifier: GPL-2.0+

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_ARCH_ARM32_ARM64 True)
set(CONFIG_CHIP_SUN50IW9 True)
set(CONFIG_CHIP_WITHPMU True)
set(CONFIG_CHIP_GPIO_V1 True)
set(CONFIG_CHIP_MINSTACK True)
set(CONFIG_BOARD_LONGANPI-3H True)

add_definitions(-DCONFIG_CHIP_SUN50IW9 -DCONFIG_CHIP_GPIO_V1)

set(CONFIG_USE_DRAM_PAYLOAD True)
set(CONFIG_USE_PREBUILT_DRAM_PAYLOAD True)
set(CONFIG_USE_DRAM_PAYLOAD_SOURCE_PATH "${CMAKE_SOURCE_DIR}/payloads/sun50iw9_libdram")
set(CONFIG_USE_DRAM_PAYLOAD_BIN_PATH "${CONFIG_USE_DRAM_PAYLOAD_SOURCE_PATH}/output/ddr.bin")
set(CONFIG_USE_DRAM_PAYLOAD_FILE_PATH "${CMAKE_SOURCE_DIR}/board/longanpi-3h/payloads/init_dram_bin.c")
set(CONFIG_USE_DRAM_PAYLOAD_SECTION "init_dram_bin")

# Set the cross-compile toolchain
set(CROSS_COMPILE "arm-none-eabi-")
set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

# Set the C and C++ compilers
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

set(CMAKE_COMMON_FLAGS "-nostdinc -nostdlib -Os -mthumb-interwork -mthumb -march=armv8-a -Wno-builtin-declaration-mismatch -mfpu=neon -mfloat-abi=softfp -pipe")

# Disable specific warning flags for C and C++ compilers
set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-discarded-qualifiers -Wno-implicit-function-declaration")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(ARCH_BIN_START_ADDRESS "0x00020000")
set(ARCH_BIN_SRAM_LENGTH "128K")

set(ARCH_FEL_START_ADDRESS "0x00028000")
set(ARCH_FEL_SRAM_LENGTH "128K")

if(NOT CONFIG_USE_PREBUILT_DRAM_PAYLOAD)
    # Create an external project and build it
    ExternalProject_Add(
        init_dram
        PREFIX init_dram
        SOURCE_DIR "${CONFIG_USE_DRAM_PAYLOAD_SOURCE_PATH}"
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make -C ${CONFIG_USE_DRAM_PAYLOAD_SOURCE_PATH}
        BUILD_IN_SOURCE 1
    )

    # Create inital init dram bin file for build
    add_custom_command(
        TARGET init_dram
        POST_BUILD COMMAND ${CMAKE_BIN2ARRAY} ${CONFIG_USE_DRAM_PAYLOAD_BIN_PATH} ${CONFIG_USE_DRAM_PAYLOAD_FILE_PATH} ${CONFIG_USE_DRAM_PAYLOAD_SECTION}
        COMMENT "Generate DRAM LIB Payload ${CONFIG_USE_DRAM_PAYLOAD_BIN_PATH} for ${CONFIG_USE_DRAM_PAYLOAD_FILE_PATH}"
    )
endif()