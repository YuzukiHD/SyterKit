set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
)

add_subdirectory(hello_world)

add_subdirectory(ruapu_test)

add_subdirectory(read_chipsid)

add_subdirectory(init_dram)

add_subdirectory(read_chip_efuse)

add_subdirectory(syter_boot)

add_subdirectory(syter_boot_sdnand)

add_subdirectory(syter_bootargs)

add_subdirectory(load_e907)

add_subdirectory(syter_amp)

add_subdirectory(fdt_parser)

add_subdirectory(cli_test)

add_subdirectory(fdt_cli)

add_subdirectory(syter_boot_spi)

add_subdirectory(i2c_test)

add_subdirectory(i2c_oled)

add_subdirectory(spi_lcd)

add_subdirectory(usb_test)
