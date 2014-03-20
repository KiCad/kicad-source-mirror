include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/lib_dxf"
    "${CMAKE_SOURCE_DIR}/pcbnew/exporters"
    "${CMAKE_SOURCE_DIR}/utils/idftools"
  )

link_directories(
    "${CMAKE_BINARY_DIR}/lib_dxf"
)

add_executable( idfcyl idf_cylinder.cpp )

add_executable( idfrect idf_rect.cpp )

add_executable( dxf2idf dxf2idfmain.cpp dxf2idf.cpp
    "${CMAKE_SOURCE_DIR}/pcbnew/exporters/idf_common.cpp"
    "${CMAKE_SOURCE_DIR}/common/richio.cpp"
  )

add_dependencies( idfcyl lib-dependencies )
add_dependencies( idfrect lib-dependencies )
add_dependencies( dxf2idf lib-dependencies )

target_link_libraries( dxf2idf lib_dxf ${wxWidgets_LIBRARIES} )

install( TARGETS idfcyl idfrect dxf2idf
    DESTINATION ${KICAD_BIN}
    COMPONENT binary )
