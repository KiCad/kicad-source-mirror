include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils/idftools
)

add_library( s3d_plugin_idf MODULE
        s3d_plugin_idf.cpp
        ${CMAKE_SOURCE_DIR}/utils/idftools/vrml_layer.cpp
        )

target_link_libraries( s3d_plugin_idf kicad_3dsg idf3 ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( s3d_plugin_idf PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_PLUGIN_DIR}/3d"
    )
endif()

install( TARGETS
    s3d_plugin_idf
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )
