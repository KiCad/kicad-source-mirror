add_subdirectory( idf )

# kicad_3d_import loads inside host processes that have no KiCad DLL stack, so it links only
# wxWidgets base and xml.  Probe them in a child scope so the narrow library list does not leak
# into the vrml/ and oce/ subdirectories, which read the parent's wxWidgets_LIBRARIES.
function( kicad_find_3d_import_wx_libraries output_variable )
    find_package( wxWidgets 3.2.0 COMPONENTS base xml REQUIRED )
    set( ${output_variable} ${wxWidgets_LIBRARIES} PARENT_SCOPE )
endfunction()

kicad_find_3d_import_wx_libraries( KICAD_3D_IMPORT_WX_LIBRARIES )

# Developers may wish to set one of these parameters on debug
# builds to get informational messages from the various parser
# modules. Valid values are 1..4. Setting a value of 4 is
# generally not recommended since it causes the parsers to
# write a VRML2 equivalent copy of the input file by invoking the
# SceneGraph->WriteVRML() function and depending on the setup
# the object may attempt to write to a protected directory.
#
# add_definitions( -DDEBUG_VRML1=1 -DDEBUG_VRML2=1 -DDEBUG_X3D=1 )
#
# In addition to setting the verbosity of each parser module,
# an appropriate WXTRACE value must be set prior to program
# execution to enable the logging:
#
# export WXTRACE="PLUGIN_VRML"
#

add_library( kicad_3d_import SHARED
    model_import.cpp
    model_import_queue.cpp
    oce/loadmodel.cpp
    vrml/vrml.cpp
    vrml/vrml_line_reader.cpp
    vrml/x3d.cpp
    vrml/wrlproc.cpp
    vrml/wrlfacet.cpp
    vrml/v2/vrml2_node.cpp vrml/v2/vrml2_base.cpp vrml/v2/vrml2_transform.cpp
    vrml/v2/vrml2_shape.cpp vrml/v2/vrml2_appearance.cpp vrml/v2/vrml2_material.cpp
    vrml/v2/vrml2_faceset.cpp vrml/v2/vrml2_lineset.cpp vrml/v2/vrml2_pointset.cpp
    vrml/v2/vrml2_coords.cpp vrml/v2/vrml2_norms.cpp vrml/v2/vrml2_color.cpp
    vrml/v2/vrml2_box.cpp vrml/v2/vrml2_switch.cpp vrml/v2/vrml2_inline.cpp
    vrml/v1/vrml1_node.cpp vrml/v1/vrml1_base.cpp vrml/v1/vrml1_group.cpp
    vrml/v1/vrml1_separator.cpp vrml/v1/vrml1_material.cpp vrml/v1/vrml1_matbinding.cpp
    vrml/v1/vrml1_coords.cpp vrml/v1/vrml1_switch.cpp vrml/v1/vrml1_faceset.cpp
    vrml/v1/vrml1_transform.cpp vrml/v1/vrml1_shapehints.cpp
    vrml/x3d/x3d_appearance.cpp vrml/x3d/x3d_base.cpp vrml/x3d/x3d_coords.cpp
    vrml/x3d/x3d_ifaceset.cpp vrml/x3d/x3d_ops.cpp vrml/x3d/x3d_shape.cpp
    vrml/x3d/x3d_transform.cpp
)

set( KICAD_3D_IMPORT_OCC_NAMES
    TKernel TKMath TKG2d TKG3d TKBRep TKGeomBase TKGeomAlgo TKTopAlgo TKPrim TKBO
    TKShHealing TKMesh TKCDF TKLCAF TKCAF TKTObj TKXCAF TKXSBase
)

if( OCC_VERSION_STRING VERSION_LESS "7.8.0" )
    list( APPEND KICAD_3D_IMPORT_OCC_NAMES
        TKIGES TKSTEP209 TKSTEPAttr TKSTEPBase TKSTEP TKXDEIGES TKXDESTEP
    )
else()
    list( APPEND KICAD_3D_IMPORT_OCC_NAMES TKDEIGES TKDESTEP )
endif()

set( KICAD_3D_IMPORT_OCC_LIBRARIES )

foreach( OCC_LIBRARY IN LISTS OCC_LIBRARIES )
    get_filename_component( OCC_LIBRARY_NAME "${OCC_LIBRARY}" NAME_WE )
    string( REGEX REPLACE "^lib" "" OCC_LIBRARY_NAME "${OCC_LIBRARY_NAME}" )

    if( OCC_LIBRARY_NAME IN_LIST KICAD_3D_IMPORT_OCC_NAMES )
        list( APPEND KICAD_3D_IMPORT_OCC_LIBRARIES "${OCC_LIBRARY}" )
    endif()
endforeach()

target_compile_definitions( kicad_3d_import PRIVATE COMPILE_3D_IMPORT )
target_link_libraries( kicad_3d_import PRIVATE kicad_3dsg
    ${KICAD_3D_IMPORT_OCC_LIBRARIES} ${KICAD_3D_IMPORT_WX_LIBRARIES} ZLIB::ZLIB )

if( WIN32 )
    target_link_libraries( kicad_3d_import PRIVATE shell32 ole32 advapi32 )
endif()

target_include_directories( kicad_3d_import PRIVATE
    $<TARGET_PROPERTY:gzip-hpp,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/vrml
    ${CMAKE_CURRENT_SOURCE_DIR}/vrml/v1
    ${CMAKE_CURRENT_SOURCE_DIR}/vrml/v2
    ${CMAKE_CURRENT_SOURCE_DIR}/vrml/x3d
)

if( APPLE )
    set_target_properties( kicad_3d_import PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
        INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
    )
endif()

install( TARGETS kicad_3d_import
    RUNTIME DESTINATION ${KICAD_LIB}
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
)

if( KICAD_WIN32_INSTALL_PDBS )
    install( FILES $<TARGET_PDB_FILE:kicad_3d_import> DESTINATION ${KICAD_BIN} )
endif()

add_subdirectory( vrml )
add_subdirectory( oce )
