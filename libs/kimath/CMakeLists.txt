set( KIMATH_SRCS
    src/bezier_curves.cpp
    src/convert_basic_shapes_to_polygon.cpp
    src/md5_hash.cpp
    src/trigo.cpp

    src/geometry/circle.cpp
    src/geometry/convex_hull.cpp
    src/geometry/direction_45.cpp
    src/geometry/geometry_utils.cpp
    src/geometry/poly_grid_partition.cpp
    src/geometry/seg.cpp
    src/geometry/shape.cpp
    src/geometry/shape_arc.cpp
    src/geometry/shape_collisions.cpp
    src/geometry/shape_file_io.cpp
    src/geometry/shape_line_chain.cpp
    src/geometry/shape_poly_set.cpp
    src/geometry/shape_rect.cpp
    src/geometry/shape_compound.cpp
    src/geometry/shape_segment.cpp


    src/math/util.cpp
)

# Include the other smaller math libraries in this one for convenience
add_library( kimath STATIC
    ${KIMATH_SRCS}
    $<TARGET_OBJECTS:clipper>
    $<TARGET_OBJECTS:othermath>
)

add_dependencies( kimath clipper )
add_dependencies( kimath rtree )
add_dependencies( kimath othermath )

target_link_libraries( kimath
    ${wxWidgets_LIBRARIES}      # wxLogDebug, wxASSERT
    ${Boost_LIBRARIES}          # Because of the OPT types
)

target_include_directories( kimath PUBLIC
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    $<TARGET_PROPERTY:clipper,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:othermath,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:rtree,INTERFACE_INCLUDE_DIRECTORIES>
)

# core/optional.h is needed for OPT types
target_include_directories( kimath PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${wxWidgets_LIBRARIES}
    ${Boost_INCLUDE_DIR}
)
