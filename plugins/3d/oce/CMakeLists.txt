# Developers may wish to set DEBUG_OCE to a value from 1..3 to
# enable informational messages in Debug build. Setting a value
# of 4 or greater will instruct the parser to write a VRML2
# equivalent copy of the input file by invoking the
# SceneGraph->WriteVRML() function and depending on the setup
# the object may attempt to write to a protected directory.
#
# In addition to setting the verbosity via DEBUG_OCE, an
# appropriate WXTRACE value must be set prior to program
# execution to enable the logging:
#
# export WXTRACE="PLUGIN_OCE"
#

add_library( s3d_plugin_oce MODULE
        oce.cpp
        loadmodel.cpp
        )

target_link_libraries( s3d_plugin_oce kicad_3dsg common ${OCC_LIBRARIES} ${wxWidgets_LIBRARIES} ZLIB::ZLIB )

target_include_directories( s3d_plugin_oce PRIVATE
    $<TARGET_PROPERTY:gzip-hpp,INTERFACE_INCLUDE_DIRECTORIES>
    )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( s3d_plugin_oce PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_PLUGIN_DIR}/3d"
    )
endif()

install( TARGETS
    s3d_plugin_oce
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:s3d_plugin_oce> DESTINATION ${KICAD_USER_PLUGIN}/3d)
endif()
