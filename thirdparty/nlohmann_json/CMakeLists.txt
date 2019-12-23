add_library( nlohmann_json INTERFACE )

target_include_directories( nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( nlohmann_json INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann/json.hpp
    )