# SPDX-License-Identifier: Apache-2.0

# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

include(ExternalProject)

# Define the path to the board loader files
set(BOARD_FLOADER_PATH ${CMAKE_SOURCE_DIR}/cmake/board/)
# Define the path to the mksunxi tool
set(CMAKE_MKSUNXI "${CMAKE_SOURCE_DIR}/tools/mksunxi")
# Define the path to the bin2array tool
set(CMAKE_BIN2ARRAY "${CMAKE_SOURCE_DIR}/tools/bin2array")

# Check if the CMAKE_BOARD_FILE variable is defined
if(CMAKE_BOARD_FILE)
    # Set the value of CMAKE_BOARD_FILE and include the file
    set(CMAKE_BOARD_FILE ${BOARD_FLOADER_PATH}${CMAKE_BOARD_FILE})
    message(STATUS "CMAKE_BOARD_FILE = ${CMAKE_BOARD_FILE}")
    include(${CMAKE_BOARD_FILE})
else()
    set(CMAKE_AVALIABLE_BOARD "")
    file(GLOB FILE_LIST "${BOARD_FLOADER_PATH}/*.cmake")
    foreach(FILE_PATH ${FILE_LIST})
        get_filename_component(BOARD_FILE_NAME ${FILE_PATH} NAME)
        set(CMAKE_AVALIABLE_BOARD "${CMAKE_AVALIABLE_BOARD}\n ${BOARD_FILE_NAME}")
    endforeach()
    message(FATAL_ERROR 
        "You need to select target Board:" 
        "${CMAKE_AVALIABLE_BOARD}" 
        "\nUse -DCMAKE_BOARD_FILE=xxx.cmake to define target board"
        "\neg:"
        "\n cmake -DCMAKE_BOARD_FILE=tinyvision.cmake .."
    )
endif()

# Set compilation flags
set(CMAKE_C_FLAGS "${CMAKE_C_DISABLE_WARN_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_DISABLE_WARN_FLAGS} ${CMAKE_CXX_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "c++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_DISABLE_WARN_FLAGS} ${CMAKE_ASM_FLAGS} ${CMAKE_COMMON_FLAGS}" CACHE STRING "asm flags")

# Define the major and minor version numbers of the project
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)

# Use Git commands to get the Git version and hash values of the project
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

# Define the project name and languages used
project(SyterKit C CXX ASM)

# Include other CMake modules
include(cmake/add_syterkit_app.cmake)

# Get the name of the compiler
string(SUBSTRING ${CMAKE_C_COMPILER} 1 -1 COMPILER_NAME)
string(REGEX REPLACE ".*/" "" COMPILER_NAME ${COMPILER_NAME})

# Configure file as required
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

# Create an external project and build it
ExternalProject_Add(
    mksunxi
    PREFIX mksunxi
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/tools"
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -C ${PROJECT_SOURCE_DIR}/tools
    BUILD_IN_SOURCE 1
)

# Set tools required for the build process
set(CMAKE_AR "${CROSS_COMPILE}ar")
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy")
set(CMAKE_SIZE "${CROSS_COMPILE}size")

# Define the paths to the linker scripts
set(LINK_SCRIPT_FEL ${PROJECT_BINARY_DIR}/link_elf.ld)
set(LINK_SCRIPT_BIN ${PROJECT_BINARY_DIR}/link_bin.ld)

# If the CONFIG_ARCH_ARM32 variable is defined, execute the following content
if (CONFIG_ARCH_ARM32)
set(ARCH_INCLUDE include/arch/arm32)


set(ARCH_START_ADDRESS "${ARCH_BIN_START_ADDRESS}")
set(ARCH_SRAM_LENGTH "${ARCH_BIN_SRAM_LENGTH}")

configure_file(
    "${PROJECT_SOURCE_DIR}/link/arm32/link.ld"
    "${PROJECT_BINARY_DIR}/link_bin.ld"
)

set(ARCH_START_ADDRESS "${ARCH_FEL_START_ADDRESS}")
set(ARCH_SRAM_LENGTH "${ARCH_FEL_SRAM_LENGTH}")

configure_file(
    "${PROJECT_SOURCE_DIR}/link/arm32/link.ld"
    "${PROJECT_BINARY_DIR}/link_elf.ld"
)
endif()

# If the CONFIG_ARCH_ARM32 variable is defined, execute the following content
if (CONFIG_ARCH_RISCV64)
set(ARCH_INCLUDE include/arch/riscv64)

set(ARCH_START_ADDRESS "${ARCH_BIN_START_ADDRESS}")
set(ARCH_SRAM_LENGTH "${ARCH_BIN_SRAM_LENGTH}")

configure_file(
    "${PROJECT_SOURCE_DIR}/link/riscv64/link.ld"
    "${PROJECT_BINARY_DIR}/link_bin.ld"
)

set(ARCH_START_ADDRESS "${ARCH_FEL_START_ADDRESS}")
set(ARCH_SRAM_LENGTH "${ARCH_FEL_SRAM_LENGTH}")

configure_file(
    "${PROJECT_SOURCE_DIR}/link/riscv64/link.ld"
    "${PROJECT_BINARY_DIR}/link_elf.ld"
)
endif()

# Specify the paths of the include files
include_directories(
    include
    include/cli
    include/drivers
    include/drivers/reg
    include/lib/fatfs
    include/lib/fdt
    include/lib/elf
    include/lib/ini
    ${ARCH_INCLUDE}
    ${PROJECT_BINARY_DIR}
)

# Add subdirectories and execute the appropriate CMakeLists.txt files
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(board)
