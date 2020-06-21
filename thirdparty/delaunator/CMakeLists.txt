set(DELAUNATOR_SRCS
    delaunator.cpp
    )

add_library(delaunator STATIC ${DELAUNATOR_SRCS})

target_include_directories( delaunator
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
