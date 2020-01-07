
set( TTL_SRCS
    src/halfedge/hetriang.cpp
)

add_library( ttl STATIC
    ${TTL_SRCS}
)

target_include_directories( ttl
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# It needs VECTOR2D from the math library
# Luckily that is header-only
target_include_directories( ttl
    PRIVATE
    ${PROJECT_SOURCE_DIR}/libs/kimath/include
)
