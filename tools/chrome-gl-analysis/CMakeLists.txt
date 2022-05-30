# add_executable(chrome-gl-analysis chrome-gl-analysis.cpp)
# target_link_libraries(chrome-gl-analysis ${llvm_libs})
# set_target_properties(chrome-gl-analysis PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(api_func_parse api_func_parse.cpp util.cpp)
target_link_libraries(api_func_parse Svf Cudd ${llvm_libs} nlohmann_json::nlohmann_json)
set_target_properties(api_func_parse PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
