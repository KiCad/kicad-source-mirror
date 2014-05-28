include_directories(
    "${CMAKE_SOURCE_DIR}/lib_dxf"
    "${CMAKE_SOURCE_DIR}/utils/idftools"
  )

link_directories(
    "${CMAKE_BINARY_DIR}/lib_dxf"
  )

add_library( idf3 STATIC
    idf_helpers.cpp idf_common.cpp idf_outlines.cpp
    idf_parser.cpp vrml_layer.cpp )

add_executable( idfcyl idf_cylinder.cpp )
add_executable( idfrect idf_rect.cpp )
add_executable( dxf2idf dxf2idfmain.cpp dxf2idf.cpp )
add_executable( idf2vrml idf2vrml.cpp )

target_link_libraries( dxf2idf lib_dxf idf3 ${wxWidgets_LIBRARIES} )

if( WIN32 )
    set ( LIB_GLU glu32 )
else()
    set ( LIB_GLU GLU )
endif()

target_link_libraries( idf2vrml idf3 ${LIB_GLU} ${wxWidgets_LIBRARIES} )


install( TARGETS idfcyl idfrect dxf2idf idf2vrml
    DESTINATION ${KICAD_BIN}
    COMPONENT binary )
