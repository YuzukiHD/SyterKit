add_library(elf
    elf32.c
    elf64.c
)

target_link_libraries(elf PRIVATE gcc)