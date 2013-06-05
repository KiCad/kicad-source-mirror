include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${INC_AFTER}
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

add_library(potrace STATIC ${POTRACE_SRCS})
