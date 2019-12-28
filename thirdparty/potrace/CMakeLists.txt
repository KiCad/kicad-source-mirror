set(POTRACE_SRCS
    src/bitmap_io.cpp
    src/curve.cpp
    src/decompose.cpp
    src/greymap.cpp
    src/potracelib.cpp
    src/render.cpp
    src/trace.cpp
    )

add_library( potrace STATIC ${POTRACE_SRCS} )

target_include_directories( potrace
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
