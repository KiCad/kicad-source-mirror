include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/utils/idftools"
    ${OPENGL_INCLUDE_DIR}
)

set( IDF3_FILES
    idf_helpers.cpp
    idf_common.cpp
    idf_outlines.cpp
    idf_parser.cpp
    vrml_layer.cpp
)

if( MINGW )
    list( APPEND IDF3_FILES ${CMAKE_SOURCE_DIR}/common/streamwrapper.cpp )
endif( MINGW )

add_library( idf3 STATIC ${IDF3_FILES} )
target_link_libraries( idf3
    kimath
    Boost::headers
    )

add_executable( idfcyl idf_cylinder.cpp )
add_executable( idfrect idf_rect.cpp )
add_executable( dxf2idf dxf2idfmain.cpp dxf2idf.cpp )
add_executable( idf2vrml idf2vrml.cpp )

target_link_libraries( dxf2idf
    dxflib_qcad
    idf3
    kimath
    common
    ${OPENGL_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    )

target_link_libraries( idf2vrml
    idf3
    kimath
    common
    ${OPENGL_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    )

if( APPLE )
    # puts binaries into the *.app bundle while linking
    set_target_properties( idfcyl idfrect dxf2idf idf2vrml PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
        )
    set_target_properties( idfcyl idfrect dxf2idf idf2vrml PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks" )
    set_target_properties( idfcyl idfrect dxf2idf idf2vrml PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )
else()
    install( TARGETS idfcyl idfrect idf2vrml
        dxf2idf
        DESTINATION ${KICAD_BIN}
        COMPONENT binary )
endif()
