
add_library( othermath OBJECT
    math_for_graphics.cpp
)

target_include_directories( othermath
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# kiround is needed
target_include_directories( othermath
    PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/libs/kimath/include
)
