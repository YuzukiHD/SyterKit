add_subdirectory(arch)
add_subdirectory(drivers)

add_library(SyterKit
    # String
    string.c

    # log
    log/log.c
    log/xformat.c

    # uart
    uart.c

    $<TARGET_OBJECTS:arch-obj>
    $<TARGET_OBJECTS:drivers-obj>
    $<TARGET_OBJECTS:chip_drivers-obj>
)