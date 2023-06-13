add_library( magic_enum INTERFACE )

target_include_directories( magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum )

target_sources( magic_enum INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/magic_enum.hpp
    )
