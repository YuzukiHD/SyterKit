add_subdirectory(sys)

add_executable(c906.elf
    start.S
    main.c
    uart.c
)

set_target_properties(c906.elf PROPERTIES LINK_DEPENDS "${LINK_SCRIPT}")
target_link_libraries(c906.elf PRIVATE libstring libsys)
