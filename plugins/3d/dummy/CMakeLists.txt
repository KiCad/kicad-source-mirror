include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library( s3d_plugin_dummy MODULE
    s3d_plugin_dummy.cpp
    )

target_link_libraries( s3d_plugin_dummy ${wxWidgets_LIBRARIES} )

if( APPLE )
    install( TARGETS
            s3d_plugin_dummy
            DESTINATION /Applications/kicad.app/SharedSupport/plugins/3d
            COMPONENT binary
    )
else()
    install( TARGETS
        s3d_plugin_dummy
        DESTINATION lib/kicad/plugins/3d
        COMPONENT binary
    )
endif()
