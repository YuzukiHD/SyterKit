# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Options

# By setting ENABLE_HARDFP to ON, it indicates that the project is configured
# to utilize hard floating-point operations when applicable. This can be beneficial 
# in scenarios where performance gains from hardware acceleration are desired.
option(ENABLE_HARDFP "Enable hardware floating-point operations" ON)

set(CROSS_COMPILE "arm-none-eabi-")

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "CROSS_COMPILE Toolchain")

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")

if(ENABLE_HARDFP)
set(CMAKE_COMMON_FLAGS "-nostdlib -g -ggdb -O3 -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard")
else()
set(CMAKE_COMMON_FLAGS "-nostdlib -g -ggdb -O3 -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=softfp")
endif()

set(CMAKE_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")

set(CMAKE_C_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_CXX_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_DISABLE_WARN_FLAGS} ${CMAKE_ASM_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "asm flags")

# Set Version, Release need cmake
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)

# use git version as library version
find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
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
include(ExternalProject)

string(SUBSTRING ${CMAKE_C_COMPILER} 1 -1 COMPILER_NAME)
string(REGEX REPLACE ".*/" "" COMPILER_NAME ${COMPILER_NAME})

configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

ExternalProject_Add(
    mksunxi
    PREFIX mksunxi
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/tools"
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -C ${PROJECT_SOURCE_DIR}/tools
    BUILD_IN_SOURCE 1
)

set(CMAKE_AR "${CROSS_COMPILE}ar")
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy")
set(CMAKE_SIZE "${CROSS_COMPILE}size")

set(CMAKE_MKSUNXI "${PROJECT_SOURCE_DIR}/tools/mksunxi")

set(LINK_SCRIPT_FEL ${CMAKE_SOURCE_DIR}/link/link_elf.ld)
set(LINK_SCRIPT_BIN ${CMAKE_SOURCE_DIR}/link/link_bin.ld)

include_directories(
    include
    include/cli
    include/arch
    include/drivers
    include/reg
    include/lib/fatfs
    include/lib/fdt
    include/lib/elf
    include/lib/ini
    ${PROJECT_BINARY_DIR}
)

add_library(SyterKit
    # log
    src/log/log.c
    src/log/xformat.c
    # string
    src/string/string.c
    src/string/memcpy.S
    src/string/memset.S
    src/string/memcmp.S
    # cli
    src/cli/commands.c
    src/cli/history.c
    src/cli/lineedit.c
    src/cli/parse.c
    src/cli/shell.c
    # exception & common
    src/exception.c
    src/common.c
    # src
    src/jmp.c
    src/uart.c
    src/ctype.c
    src/fdt_wrapper.c
    src/smalloc.c
    src/cache.S
    # deiver
    drivers/sys-clk.c
    drivers/sys-dma.c
    drivers/sys-gpio.c
    drivers/sys-dram.c
    drivers/sys-sdcard.c
    drivers/sys-sdhci.c
    drivers/sys-rproc.c
    drivers/sys-spi.c
    drivers/sys-sid.c
    drivers/sys-timer.c
    drivers/sys-uart.c
    drivers/sys-wdt.c
)

target_link_libraries(SyterKit gcc fdt)

add_subdirectory(lib)

add_subdirectory(app)