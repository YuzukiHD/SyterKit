# SPDX-License-Identifier: Apache-2.0 

add_library(arch-obj OBJECT
    timer.c
    exception.c
    memcpy.S
    memset.S
    fprw.S
    memcmp.c
)