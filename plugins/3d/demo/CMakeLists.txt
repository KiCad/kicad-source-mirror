add_library( s3d_plugin_demo1 MODULE
    s3d_plugin_demo1.cpp
    )

target_link_libraries( s3d_plugin_demo1 ${wxWidgets_LIBRARIES} )


add_library( s3d_plugin_demo2 MODULE
        s3d_plugin_demo2.cpp
        )

target_link_libraries( s3d_plugin_demo2 kicad_3dsg ${wxWidgets_LIBRARIES} )

install( TARGETS
    s3d_plugin_demo1
    s3d_plugin_demo2
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )
