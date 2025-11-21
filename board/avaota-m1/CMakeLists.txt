set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
)

set(APP_LINK_LIBRARY
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libdram.a
)

add_subdirectory(hello_world)
