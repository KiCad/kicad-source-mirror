
include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${INC_AFTER}
    )

set(POLYGON_SRCS
    math_for_graphics.cpp
    PolyLine.cpp
    polygon_test_point_inside.cpp
    clipper.cpp
)

add_library(polygon STATIC ${POLYGON_SRCS})

add_dependencies( polygon lib-dependencies )

