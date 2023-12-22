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

set(CMAKE_C_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers")
set(CMAKE_CXX_DISABLE_WARN_FLAGS "-Wno-int-to-pointer-cast")

set(CMAKE_C_FLAGS "${CMAKE_C_DISABLE_WARN_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_DISABLE_WARN_FLAGS} ${CMAKE_CXX_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_DISABLE_WARN_FLAGS} ${CMAKE_ASM_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "asm flags")

# Set Version, Release need cmake
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)

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


set(CONFIG_ARCH_ARM32 True)
set(CONFIG_CHIP_SUN8IW21 True)

add_definitions(-DCONFIG_CHIP_SUN8IW21)

if (CONFIG_ARCH_ARM32)
set(ARCH_INCLUDE include/arch/arm32)
endif()

include_directories(
    include
    include/cli
    include/drivers
    include/drivers/reg
    ${ARCH_INCLUDE}
)

add_subdirectory(src)

add_subdirectory(board/tinyvision/app/helloworld)