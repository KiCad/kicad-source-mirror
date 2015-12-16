include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library( s3d_plugin_vrml MODULE
        vrml.cpp
        vrml1.cpp
        vrml2.cpp
        )

target_link_libraries( s3d_plugin_vrml kicad_3dsg ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} )

install( TARGETS
    s3d_plugin_vrml
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )
