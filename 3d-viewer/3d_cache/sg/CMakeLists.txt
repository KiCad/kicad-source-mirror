include_directories(
    ${CMAKE_SOURCE_DIR}/include
#    ${CMAKE_SOURCE_DIR}/include/gal/opengl
    ${CMAKE_SOURCE_DIR}/3d-viewer
  )

add_library( s3d_sg SHARED
    sg_base.cpp
    sg_node.cpp
    sg_helpers.cpp
    scenegraph.cpp
    sg_appearance.cpp
    sg_faceset.cpp
    sg_shape.cpp
    sg_colors.cpp
    sg_coords.cpp
    sg_normals.cpp
    sg_index.cpp
    sg_colorindex.cpp
    sg_coordindex.cpp
    ifsg_node.cpp
    ifsg_transform.cpp
    ifsg_appearance.cpp
    ifsg_index.cpp
    ifsg_colorindex.cpp
    ifsg_coordindex.cpp
    ifsg_colors.cpp
    ifsg_coords.cpp
    ifsg_faceset.cpp
    ifsg_normals.cpp
    ifsg_shape.cpp
    ifsg_api.cpp
)

# Define a flag to expose the appropriate EXPORT macro at build time
target_compile_definitions( s3d_sg PRIVATE -DCOMPILE_SGLIB )

target_link_libraries( s3d_sg ${wxWidgets_LIBRARIES} )

if( APPLE )
        install( TARGETS
                s3d_sg
                DESTINATION /Applications/kicad.app/SharedSupport
                COMPONENT binary
        )
else()
        install( TARGETS
                s3d_sg
                DESTINATION lib/kicad
                COMPONENT binary
        )
endif()
