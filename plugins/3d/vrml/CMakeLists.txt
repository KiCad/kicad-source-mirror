include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/v1
    ${CMAKE_CURRENT_SOURCE_DIR}/v2
    ${CMAKE_CURRENT_SOURCE_DIR}/x3d
)

# Developers may wish to set one of these parameters on debug
# builds to get informational messages from the various parser
# modules. Valid values are 1..4. Setting a value of 4 is
# generally not recommended since it causes the parsers to
# write a VRML2 equivalent copy of the input file by invoking the
# SceneGraph->WriteVRML() function and depending on the setup
# the object may attempt to write to a protected directory.
#
# add_definitions( -DDEBUG_VRML1=1 -DDEBUG_VRML2=1 -DDEBUG_X3D=1 )
#
# In addition to setting the verbosity of each parser module,
# an appropriate WXTRACE value must be set prior to program
# execution to enable the logging:
#
# export WXTRACE="PLUGIN_VRML"
#

add_library( s3d_plugin_vrml MODULE
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
        v2/vrml2_inline.cpp
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

target_link_libraries( s3d_plugin_vrml kicad_3dsg kicommon core ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} kiplatform ZLIB::ZLIB )

target_include_directories( s3d_plugin_vrml PRIVATE
    $<TARGET_PROPERTY:gzip-hpp,INTERFACE_INCLUDE_DIRECTORIES>
    )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( s3d_plugin_vrml PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_PLUGIN_DIR}/3d"
    )
endif()

install( TARGETS
    s3d_plugin_vrml
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:s3d_plugin_vrml> DESTINATION ${KICAD_USER_PLUGIN}/3d)
endif()
