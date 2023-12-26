


add_executable(sun50iw9_libdram
    sys-dram.c
    memset.S
    memcpy.S
)

target_link_libraries(sun50iw9_libdram gcc SyterKit ${CMAKE_SOURCE_DIR}/libdram.a ${CMAKE_SOURCE_DIR}/libchipid.a)