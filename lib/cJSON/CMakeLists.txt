add_library(cJSON
    cJSON.c
    cJSON_Utils.c
)

target_link_libraries(cJSON PRIVATE gcc)