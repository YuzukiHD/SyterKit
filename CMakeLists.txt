# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

set(CROSS_COMPILE "arm-none-eabi-")

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

set(CMAKE_COMMON_FLAGS "-nostdlib -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard")
set(CMAKE_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration")

set(CMAKE_C_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_CXX_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_ASM_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "asm flags")

# Set Version, Release need cmake
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)

# use git version as library version
find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE _git_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            OUTPUT_VARIABLE PROJECT_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
else ()
    set(_git_version 0)
    set(PROJECT_GIT_HASH "")
endif ()

project(SyterKit C CXX ASM)

include(cmake/add_syterkit_app.cmake)

configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)

set(CMAKE_AR "${CROSS_COMPILE}ar")
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy")

set(LINK_SCRIPT_FEL ${CMAKE_SOURCE_DIR}/link/link_elf.ld)
set(LINK_SCRIPT_BIN ${CMAKE_SOURCE_DIR}/link/link_bin.ld)

include_directories(
    include
    include/arch
    include/drivers
    include/reg
    include/lib/fatfs
    include/lib/fdt
    ${PROJECT_BINARY_DIR}
)

add_library(SyterKit
    # log
    src/log/log.c
    src/log/xformat.c
    # string
    src/string/string.c
    src/string/memcpy.S
    # exception & common
    src/exception.c
    src/common.c
    src/jmp.c
    # deiver
    drivers/sys-clk.c
    drivers/sys-dma.c
    drivers/sys-gpio.c
    drivers/sys-dram.c
    drivers/sys-sdcard.c
    drivers/sys-sdhci.c
    drivers/sys-spi.c
    drivers/sys-sid.c
    drivers/sys-timer.c
    drivers/sys-uart.c
)

target_link_libraries(SyterKit gcc)

add_subdirectory(lib)

add_subdirectory(app)