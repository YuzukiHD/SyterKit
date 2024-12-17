set(GEN_CONFIG_FILE "${CMAKE_BINARY_DIR}/syterkit_cmake_variables.txt")
set(GEN_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/.vscode/c_cpp_properties.json")

# for auto generate VSCode Config
file(WRITE "${GEN_CONFIG_FILE}")
get_cmake_property(_variable_names VARIABLES)
foreach(var_name ${_variable_names})
    set(var_value ${${var_name}})
    file(APPEND "${GEN_CONFIG_FILE}" "${var_name} = ${var_value}\n")
endforeach()

add_custom_command(
    OUTPUT ${GEN_OUTPUT_FILE}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/gen_vscode_json.py ${GEN_CONFIG_FILE} ${GEN_OUTPUT_FILE}
    DEPENDS ${GEN_CONFIG_FILE}
    COMMENT "Running Python script to generate c_cpp_properties.json"
)

add_custom_target(run_python_script ALL DEPENDS ${GEN_OUTPUT_FILE})
