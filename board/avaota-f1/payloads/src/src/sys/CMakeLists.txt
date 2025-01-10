add_library(libsys
    sys-clock.c
    sys-uart.c
)

target_link_libraries(libsys PRIVATE libstring)