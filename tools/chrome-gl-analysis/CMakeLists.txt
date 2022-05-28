# add_executable(chrome-gl-analysis chrome-gl-analysis.cpp)
# target_link_libraries(chrome-gl-analysis ${llvm_libs})
# set_target_properties(chrome-gl-analysis PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(api_analyze api_analyze.cpp)
target_link_libraries(api_analyze Svf Cudd ${llvm_libs} nlohmann_json::nlohmann_json)
set_target_properties(api_analyze PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)