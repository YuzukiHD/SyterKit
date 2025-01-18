set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/head.c
    ${CMAKE_CURRENT_SOURCE_DIR}/payloads/boot.c
)

add_subdirectory(app)

add_subdirectory(boot)
