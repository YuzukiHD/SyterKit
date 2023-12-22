# SPDX-License-Identifier: Apache-2.0

set(CONFIG_ARCH_ARM32 True)
set(CONFIG_CHIP_SUN8IW21 True)

add_definitions(-DCONFIG_CHIP_SUN8IW21)

add_syterkit_app(helloworld
    ../common/start.S
    main.c
    ../common/eabi_compat.c
)