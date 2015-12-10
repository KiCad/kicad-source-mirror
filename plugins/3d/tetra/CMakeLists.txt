include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library( s3d_plugin_tetra MODULE
        s3d_plugin_tetra.cpp
        )

target_link_libraries( s3d_plugin_tetra s3d_sg ${wxWidgets_LIBRARIES} )

if( APPLE )
    install( TARGETS
            s3d_plugin_tetra
            DESTINATION /Applications/kicad.app/SharedSupport/plugins/3d
            COMPONENT binary
    )
else()
    install( TARGETS
        s3d_plugin_tetra
        DESTINATION lib/kicad/plugins/3d
        COMPONENT binary
    )
endif()
