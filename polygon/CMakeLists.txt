# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

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

