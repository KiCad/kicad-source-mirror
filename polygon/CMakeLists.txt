set(POLYGON_SRCS
    GenericPolygonClipperLibrary.cpp
    math_for_graphics.cpp
    php_polygon.cpp
    php_polygon_vertex.cpp
    PolyLine.cpp)

add_library(polygon ${POLYGON_SRCS})
