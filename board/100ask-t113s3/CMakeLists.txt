set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
)

add_subdirectory(hello_world)

add_subdirectory(load_hifi4)

add_subdirectory(usb_test)

add_subdirectory(syter_boot_rtt)