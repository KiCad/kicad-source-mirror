include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    )

set(POTRACE_SRCS
    bitmap_io.cpp
    curve.cpp
    decompose.cpp
    greymap.cpp
    potracelib.cpp
    render.cpp
    trace.cpp
    )

add_library(potrace ${POTRACE_SRCS})
