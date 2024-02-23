set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
)

add_subdirectory(hello_world)

add_subdirectory(sys_info)

add_subdirectory(syter_boot)

add_subdirectory(load_c906)

add_subdirectory(load_hifi4)

add_subdirectory(os_test)
