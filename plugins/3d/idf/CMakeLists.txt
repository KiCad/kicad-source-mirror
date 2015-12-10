include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils/idftools
)

add_library( s3d_plugin_idf MODULE
        s3d_plugin_idf.cpp
        ${CMAKE_SOURCE_DIR}/utils/idftools/vrml_layer.cpp
        )

set_target_properties( s3d_plugin_idf PROPERTIES
        LINK_FLAGS -L${CMAKE_BINARY_DIR}/utils/idftools )

target_link_libraries( s3d_plugin_idf s3d_sg idf3 ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} )

if( APPLE )
    install( TARGETS
            s3d_plugin_idf
            DESTINATION /Applications/kicad.app/SharedSupport/plugins/3d
            COMPONENT binary
    )
else()
    install( TARGETS
        s3d_plugin_idf
        DESTINATION lib/kicad/plugins/3d
        COMPONENT binary
    )
endif()
