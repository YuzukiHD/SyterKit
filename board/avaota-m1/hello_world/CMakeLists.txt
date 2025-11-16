# SPDX-License-Identifier: GPL-2.0+

set(APP_LINK_LIBRARY
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libdram.a
)

add_syterkit_app(helloworld 
    main.c
)
