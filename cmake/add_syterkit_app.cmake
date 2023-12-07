# SPDX-License-Identifier: Apache-2.0

function(add_syterkit_app target_name)
    add_executable(${target_name}_fel ${ARGN})

    set_target_properties(${target_name}_fel PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_FEL}")
    target_link_libraries(${target_name}_fel fatfs fdt SyterKit elf gcc -T"${LINK_SCRIPT_FEL}" -nostdlib -Wl,-z,noexecstack,-Map,${target_name}_fel.map)

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

    add_executable(${target_name}_bin ${ARGN})

    set_target_properties(${target_name}_bin PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_BIN}")
    target_link_libraries(${target_name}_bin fatfs fdt elf SyterKit gcc -T"${LINK_SCRIPT_BIN}" -nostdlib -Wl,-z,noexecstack,-Map,${target_name}_bin.map)

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
endfunction()