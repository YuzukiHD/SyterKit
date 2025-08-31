# SPDX-License-Identifier: GPL-2.0+

# List of common libraries used by the application
set(APP_COMMON_LIBRARY 
    fatfs
    fdt
    SyterKit
    elf
)

set(APP_LIBS
    gcc
)

# Function to add a new SyterKit application
function(add_syterkit_app target_name)
    # Add an executable for the target_name with _fel suffix
    add_executable(${target_name}_fel ${APP_COMMON_SOURCE} ${ARGN})

    # Set properties for the target, including link dependencies and libraries
    set_target_properties(${target_name}_fel PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_FEL}")
    target_link_libraries(${target_name}_fel ${APP_LIBS} -Wl,--whole-archive ${APP_COMMON_LIBRARY} ${APP_LINK_LIBRARY} -Wl,--no-whole-archive -T"${LINK_SCRIPT_FEL}" -flto -nostdlib -Wl,-gc-sections -Wl,-z,noexecstack,-Map,${target_name}_fel.map)

    # Add custom commands for post-build actions
    add_custom_command(
        TARGET ${target_name}_fel
        POST_BUILD COMMAND ${CMAKE_SIZE} -B -x ${target_name}_fel
        COMMENT "Get Size of ${target_name}_fel"
    )

    add_custom_command(
        TARGET ${target_name}_fel
        POST_BUILD COMMAND ${CMAKE_OBJCOPY} -v -O binary ${target_name}_fel ${target_name}_fel.elf
        COMMENT "Copy Binary ${target_name}_fel"
    )

    # Repeat the above steps for the _bin version of the target
    add_executable(${target_name}_bin ${APP_COMMON_SOURCE} ${ARGN})

    set_target_properties(${target_name}_bin PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_BIN}")
    target_link_libraries(${target_name}_bin ${APP_LIBS} -Wl,--whole-archive ${APP_COMMON_LIBRARY} ${APP_LINK_LIBRARY} -Wl,--no-whole-archive -T"${LINK_SCRIPT_BIN}" -flto -nostdlib -Wl,-gc-sections -Wl,-z,noexecstack,-Map,${target_name}_bin.map)

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_SIZE} -B -x ${target_name}_bin
        COMMENT "Get Size of ${target_name}_bin"
    )

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_OBJCOPY} -v -O binary ${target_name}_bin ${target_name}_bin_card.bin 
        COMMENT "Copy Block Binary ${target_name}_bin => ${target_name}_card.bin"
    )

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_OBJCOPY} -v -O binary ${target_name}_bin ${target_name}_bin_spi.bin 
        COMMENT "Copy MTD Binary ${target_name}_bin => ${target_name}_spi.bin"
    )

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_MKSUNXI} ${target_name}_bin_card.bin 512
        COMMENT "Padding Block 512 Binary"
    )

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_MKSUNXI} ${target_name}_bin_spi.bin 8192
        COMMENT "Padding MTD 8192 Binary"
    )

    if(CMAKE_BUILD_TYPE STREQUAL Trace OR CMAKE_BUILD_TYPE STREQUAL Debug)
        add_custom_command(
            TARGET ${target_name}_bin
            POST_BUILD COMMAND ${CROSS_COMPILE}objdump -D ${target_name}_bin > ${target_name}_bin.asm
            COMMENT "Objdump ${target_name}_bin"
        )

        add_custom_command(
            TARGET ${target_name}_fel
            POST_BUILD COMMAND ${CROSS_COMPILE}objdump -D ${target_name}_fel > ${target_name}_fel.asm
            COMMENT "Objdump ${target_name}_fel"
        )
    endif()
endfunction()
