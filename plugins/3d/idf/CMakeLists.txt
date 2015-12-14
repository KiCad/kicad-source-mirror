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

target_link_libraries( s3d_plugin_idf kicad_3dsg idf3 ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} )

install( TARGETS
    s3d_plugin_idf
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )
