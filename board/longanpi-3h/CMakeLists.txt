set(APP_COMMON_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/start.S 
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eabi_compat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/payloads/init_dram_bin.c
)

add_subdirectory(hello_world)

add_subdirectory(init_dram)

add_subdirectory(cli_test)

add_subdirectory(arm64_test)

add_subdirectory(smhc_test)