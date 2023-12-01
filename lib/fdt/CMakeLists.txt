add_library(fdt
    fdt.c
)

target_link_libraries(fdt PRIVATE gcc)