# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

set( KIMATH_SRCS
    src/bezier_curves.cpp
    src/convert_basic_shapes_to_polygon.cpp
    src/md5_hash.cpp
    src/transform.cpp
    src/trigo.cpp

    src/geometry/corner_operations.cpp
    src/geometry/distribute.cpp
    src/geometry/eda_angle.cpp
    src/geometry/ellipse.cpp
    src/geometry/circle.cpp
    src/geometry/convex_hull.cpp
    src/geometry/direction_45.cpp
    src/geometry/geometry_utils.cpp
    src/geometry/half_line.cpp
    src/geometry/intersection.cpp
    src/geometry/line.cpp
    src/geometry/nearest.cpp
    src/geometry/oval.cpp
    src/geometry/roundrect.cpp
    src/geometry/seg.cpp
    src/geometry/shape.cpp
    src/geometry/shape_arc.cpp
    src/geometry/shape_collisions.cpp
    src/geometry/shape_compound.cpp
    src/geometry/shape_file_io.cpp
    src/geometry/shape_line_chain.cpp
    src/geometry/shape_nearest_points.cpp
    src/geometry/shape_poly_set.cpp
    src/geometry/shape_rect.cpp
    src/geometry/shape_segment.cpp
    src/geometry/vector_utils.cpp
    src/geometry/shape_utils.cpp
    src/geometry/vertex_set.cpp


    src/math/vector2.cpp
    src/math/util.cpp
)

# Include the other smaller math libraries in this one for convenience
add_library( kimath STATIC
    ${KIMATH_SRCS}
)

target_link_libraries( kimath
    core
    clipper2
    rtree
    Boost::headers
    ${wxWidgets_LIBRARIES}      # wxLogDebug, wxASSERT
)

target_include_directories( kimath PUBLIC
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    # This include REALLY shouldn't be here, but shape_arc.h grew a dependency on the units somehow
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories( kimath PRIVATE
    # This include REALLY shouldn't be here, but shape_arc grew a dependency on the units somehow
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)
