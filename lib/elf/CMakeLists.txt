add_library(elf
    elf.c
)

target_link_libraries(elf PRIVATE gcc)