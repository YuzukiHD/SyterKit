# SPDX-License-Identifier: GPL-2.0+ 

add_library(arch-obj OBJECT
    backtrace.c
    timer.c
    exception.c
    memcpy.S
    memset.S
    fprw.S
    memcmp.c
)