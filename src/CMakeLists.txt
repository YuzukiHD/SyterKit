add_subdirectory(arch)
add_subdirectory(drivers)

add_library(SyterKit STATIC
    # Common 
    common.c

    # String
    string.c

    # log
    log/log.c
    log/xformat.c

    # uart
    uart.c

    # cli
    cli/commands.c
    cli/history.c
    cli/lineedit.c
    cli/parse.c
    cli/shell.c

    # linux image loader
    image/bimage.c
    image/uimage.c
    image/zimage.c

    # os
    os.c

    # malloc
    smalloc.c

    # stdlib
    sstdlib.c

    # fdt
    fdt_wrapper.c

    # ctype
    ctype.c

    # chip driver
    $<TARGET_OBJECTS:chip_drivers-obj>

    # arch driver
    $<TARGET_OBJECTS:arch-obj>

    # common driver
    $<TARGET_OBJECTS:drivers-obj>
)

target_link_libraries(SyterKit PRIVATE fatfs fdt elf gcc)