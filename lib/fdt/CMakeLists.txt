add_library(fdt
    fdt.c
    fdt_empty_tree.c
    fdt_addresses.c
    fdt_overlay.c
    fdt_ro.c
    fdt_rw.c
    fdt_strerror.c
    fdt_sw.c
    fdt_wip.c
)

target_link_libraries(fdt PRIVATE gcc)