include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3d-viewer
)

set( SG_FILES
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
    sg_coordindex.cpp
    ifsg_node.cpp
    ifsg_transform.cpp
    ifsg_appearance.cpp
    ifsg_index.cpp
    ifsg_coordindex.cpp
    ifsg_colors.cpp
    ifsg_coords.cpp
    ifsg_faceset.cpp
    ifsg_normals.cpp
    ifsg_shape.cpp
    ifsg_api.cpp
)

if( MINGW )
    list( APPEND SG_FILES ${CMAKE_SOURCE_DIR}/common/streamwrapper.cpp )
endif( MINGW )

add_library( kicad_3dsg SHARED ${SG_FILES} )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( kicad_3dsg PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
        INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
    )
endif()

find_file( S3DSG_VERSION_FILE sg_version.h
        PATHS ${CMAKE_SOURCE_DIR}/include/plugins/3dapi NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

if( NOT ${S3DSG_VERSION_FILE} STREQUAL "S3DSG_VERSION_FILE-NOTFOUND" )

    # extract the "#define KICADSG_VERSION_*" lines
    file( STRINGS ${S3DSG_VERSION_FILE} _version
        REGEX "^[' ','\t']*#define[' ','\t']*KICADSG_VERSION_.*" )

    foreach( SVAR ${_version} )
        string( REGEX MATCH KICADSG_VERSION_[M,A,J,O,R,I,N,P,T,C,H,E,V]* _VARNAME ${SVAR} )
        string( REGEX MATCH [0-9]+ _VALUE ${SVAR} )

        if( NOT ${_VARNAME} STREQUAL "" )
            if( NOT ${_VALUE} STREQUAL "" )
                set( ${_VARNAME} ${_VALUE} )
            else()
                set( ${_VARNAME} 0 )
            endif()
        endif()

    endforeach()

    if( NOT KICADSG_VERSION_MAJOR AND NOT ${KICADSG_VERSION_MAJOR} STREQUAL "0" )
        message( FATAL_ERROR "Cannot determine the S3DSG library version" )
    endif()

    #ensure that NOT SG_VERSION* will evaluate to '0'
    if( NOT KICADSG_VERSION_MINOR )
        set( KICADSG_VERSION_MINOR 0 )
    endif()

    if( NOT KICADSG_VERSION_PATCH )
        set( KICADSG_VERSION_PATCH 0 )
    endif()

    set_target_properties( kicad_3dsg
            PROPERTIES SOVERSION
            ${KICADSG_VERSION_MAJOR}.${KICADSG_VERSION_MINOR}.${KICADSG_VERSION_PATCH} )

    message( STATUS "S3DSG version: ${KICADSG_VERSION_MAJOR}.${KICADSG_VERSION_MINOR}.${KICADSG_VERSION_PATCH}" )
else()
    message( FATAL_ERROR "Cannot determine the S3DSG library version" )
endif()

unset( S3DSG_VERSION_FILE CACHE )

# Define a flag to expose the appropriate EXPORT macro at build time
target_compile_definitions( kicad_3dsg PRIVATE COMPILE_SGLIB )

target_link_libraries( kicad_3dsg ${wxWidgets_LIBRARIES} )

# Don't specify the ARCHIVE DESTINATION parameter to prevent
# the install of the import library on Windows
# https://cmake.org/pipermail/cmake/2011-November/047746.html
install( TARGETS
    kicad_3dsg
    RUNTIME DESTINATION ${KICAD_LIB}
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:kicad_3dsg> DESTINATION ${KICAD_BIN})
endif()
