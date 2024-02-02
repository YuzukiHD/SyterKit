# SPDX-License-Identifier: Apache-2.0

add_syterkit_app(init_dram 
    main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/init_dram_bin.c
)