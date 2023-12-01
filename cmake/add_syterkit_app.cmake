# SPDX-License-Identifier: Apache-2.0

function(add_syterkit_app target_name)
    add_executable(${target_name}_fel ${ARGN})

    set_target_properties(${target_name}_fel PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_FEL}")
    target_link_libraries(${target_name}_fel fatfs fdt SyterKit gcc -T"${LINK_SCRIPT_FEL}" -nostdlib -Wl,-Map,${target_name}_fel.map)

    add_custom_command(
        TARGET ${target_name}_fel
        POST_BUILD COMMAND ${CMAKE_OBJCOPY} -v -O binary ${target_name}_fel ${target_name}_fel.elf 
        COMMENT "Copy Binary"
    )

    add_executable(${target_name}_bin ${ARGN})

    set_target_properties(${target_name}_bin PROPERTIES LINK_DEPENDS "${LINK_SCRIPT_BIN}")
    target_link_libraries(${target_name}_bin fatfs fdt SyterKit gcc -T"${LINK_SCRIPT_BIN}" -nostdlib -Wl,-Map,${target_name}_bin.map)

    add_custom_command(
        TARGET ${target_name}_bin
        POST_BUILD COMMAND ${CMAKE_OBJCOPY} -v -O binary ${target_name}_bin ${target_name}_bin.elf 
        COMMENT "Copy Binary"
    )
endfunction()