add_library( nlohmann_json_schema_validator STATIC
    json-schema-draft7.json.cpp
    json-uri.cpp
    json-validator.cpp
    json-patch.cpp
    string-format-check.cpp
)

add_dependencies( nlohmann_json_schema_validator kicommon )

target_include_directories( nlohmann_json_schema_validator
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries( nlohmann_json_schema_validator
    PUBLIC nlohmann_json
    PRIVATE kicommon
)
