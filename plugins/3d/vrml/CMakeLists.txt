add_library( s3d_plugin_vrml MODULE
        plugin.cpp
        )

target_link_libraries( s3d_plugin_vrml kicad_3d_import kicad_3dsg )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( s3d_plugin_vrml PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_PLUGIN_DIR}/3d"
    )
endif()

install( TARGETS
    s3d_plugin_vrml
    DESTINATION ${KICAD_USER_PLUGIN}/3d
    COMPONENT binary
    )

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:s3d_plugin_vrml> DESTINATION ${KICAD_USER_PLUGIN}/3d)
endif()
