SET(POLYGON_SRCS
    GenericPolygonClipperLibrary.cpp
    math_for_graphics.cpp
    php_polygon.cpp
    php_polygon_vertex.cpp
    PolyLine.cpp)

ADD_LIBRARY(polygon ${POLYGON_SRCS})
