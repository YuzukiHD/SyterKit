# SPDX-License-Identifier: GPL-2.0+ 

add_library(arch-obj OBJECT
    backtrace.c
    timer.c
    exception.c
    memcpy.S
    memset.S
    fprw.S
    cache.c
    memcmp.c
)

if(CMAKE_C_COMPILER_VERSION VERSION_LESS "11.0.0")
    add_definitions(-DCONFIG_RISCV_COMPILER_GCC_10)
endif()
