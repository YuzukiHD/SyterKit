add_library(fatfs
    diskio.c
    ff.c
    ffsystem.c
    ffunicode.c
)

target_link_libraries(fatfs PRIVATE gcc)