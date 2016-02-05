include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/v1
    ${CMAKE_CURRENT_SOURCE_DIR}/v2
    ${CMAKE_CURRENT_SOURCE_DIR}/x3d
)

add_definitions( -DDEBUG_VRML1=1 -DDEBUG_VRML2=1 -DDEBUG_X3D=1 )

add_library( s3d_plugin_vrml MODULE
        ${CMAKE_SOURCE_DIR}/common/richio.cpp
        vrml.cpp
        x3d.cpp
        wrlproc.cpp
        wrlfacet.cpp
        v2/vrml2_node.cpp
        v2/vrml2_base.cpp
        v2/vrml2_transform.cpp
        v2/vrml2_shape.cpp
        v2/vrml2_appearance.cpp
        v2/vrml2_material.cpp
        v2/vrml2_faceset.cpp
        v2/vrml2_lineset.cpp
        v2/vrml2_pointset.cpp
        v2/vrml2_coords.cpp
        v2/vrml2_norms.cpp
        v2/vrml2_color.cpp
        v2/vrml2_box.cpp
        v2/vrml2_switch.cpp
        v1/vrml1_node.cpp
        v1/vrml1_base.cpp
        v1/vrml1_group.cpp
        v1/vrml1_separator.cpp
        v1/vrml1_material.cpp
        v1/vrml1_matbinding.cpp
        v1/vrml1_coords.cpp
        v1/vrml1_switch.cpp
        v1/vrml1_faceset.cpp
        v1/vrml1_transform.cpp
        v1/vrml1_shapehints.cpp
        x3d/x3d_appearance.cpp
        x3d/x3d_base.cpp
        x3d/x3d_coords.cpp
        x3d/x3d_ifaceset.cpp
        x3d/x3d_ops.cpp
        x3d/x3d_shape.cpp
        x3d/x3d_transform.cpp
        )

target_link_libraries( s3d_plugin_vrml kicad_3dsg ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} )

install( TARGETS
    s3d_plugin_vrml
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )
