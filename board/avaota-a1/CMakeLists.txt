set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
)

set(APP_LINK_LIBRARY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libdram.a
)

add_subdirectory(hello_world)

add_subdirectory(init_dram)

add_subdirectory(syter_boot)

add_subdirectory(syter_boot_uboot)

add_subdirectory(smhc_test)

add_subdirectory(smhc2_test)

add_subdirectory(extlinux_boot)

add_subdirectory(spi_lcd)

add_subdirectory(load_e906)
