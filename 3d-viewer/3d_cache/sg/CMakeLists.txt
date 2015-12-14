include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3d-viewer
  )

add_library( kicad_3dsg SHARED
    sg_base.cpp
    sg_node.cpp
    sg_helpers.cpp
    scenegraph.cpp
    sg_appearance.cpp
    sg_faceset.cpp
    sg_shape.cpp
    sg_colors.cpp
    sg_coords.cpp
    sg_normals.cpp
    sg_index.cpp
    sg_colorindex.cpp
    sg_coordindex.cpp
    ifsg_node.cpp
    ifsg_transform.cpp
    ifsg_appearance.cpp
    ifsg_index.cpp
    ifsg_colorindex.cpp
    ifsg_coordindex.cpp
    ifsg_colors.cpp
    ifsg_coords.cpp
    ifsg_faceset.cpp
    ifsg_normals.cpp
    ifsg_shape.cpp
    ifsg_api.cpp
)

find_file( S3DSG_VERSION_FILE sg_version.h
        PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH )

if( NOT ${S3DSG_VERSION_FILE} STREQUAL "S3DSG_VERSION_FILE-NOTFOUND" )

    # extract the "#define SG_VERSION_*" lines
    file( STRINGS ${S3DSG_VERSION_FILE} _version
        REGEX "^[' ','\t']*#define[' ','\t']*SG_VERSION_.*" )

    foreach( SVAR ${_version} )
        string( REGEX MATCH SG_VERSION_[M,A,J,O,R,I,N,P,T,C,H,E,V]* _VARNAME ${SVAR} )
        string( REGEX MATCH [0-9]+ _VALUE ${SVAR} )

        if( NOT ${_VARNAME} STREQUAL "" )
            message( "INFO: " ${_VARNAME} " = " ${_VALUE} )
            if( NOT ${_VALUE} STREQUAL "" )
                set( ${_VARNAME} ${_VALUE} )
            else()
                set( ${_VARNAME} 0 )
            endif()
        endif()

    endforeach()

    if( NOT SG_VERSION_MAJOR AND NOT ${SG_VERSION_MAJOR} STREQUAL "0" )
        message( FATAL_ERROR "Cannot determine the S3DSG library version" )
    endif()

    #ensure that NOT SG_VERSION* will evaluate to '0'
    if( NOT SG_VERSION_MINOR )
        set( SG_VERSION_MINOR 0 )
    endif()

    if( NOT SG_VERSION_PATCH )
        set( SG_VERSION_PATCH 0 )
    endif()

    set_target_properties( kicad_3dsg
            PROPERTIES SOVERSION
            ${SG_VERSION_MAJOR}.${SG_VERSION_MINOR}.${SG_VERSION_PATCH} )

else()
    message( FATAL_ERROR "Cannot determine the S3DSG library version" )
endif()

unset( S3DSG_VERSION_FILE CACHE )

# Define a flag to expose the appropriate EXPORT macro at build time
target_compile_definitions( kicad_3dsg PRIVATE -DCOMPILE_SGLIB )

target_link_libraries( kicad_3dsg ${wxWidgets_LIBRARIES} )

install( TARGETS
    kicad_3dsg
    DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )
