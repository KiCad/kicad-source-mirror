
add_library( clipper OBJECT
    clipper.cpp
)

target_include_directories( clipper
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
