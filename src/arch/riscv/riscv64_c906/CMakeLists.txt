# SPDX-License-Identifier: GPL-2.0+ 

add_library(arch-obj OBJECT
    timer.c
    exception.c
    cache.c
    memcpy.S
    memset.S
    fprw.S
    memcmp.c
)