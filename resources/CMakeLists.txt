#################################################
# Generate platform metadata files
#################################################
if( APPLE )
    #TODO Generate this at runtime as well
    include( ${CMAKE_MODULE_PATH}/WritePlatformMetadata_macos.cmake )
elseif( UNIX )
    # Add the command to configure the linux metainfo file with the version information
    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
        COMMAND ${CMAKE_COMMAND}
            -DSRC_PATH=${PROJECT_SOURCE_DIR}
            -DBUILD_PATH=${PROJECT_BINARY_DIR}
            -DCMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/CMakeModules
            -P ${CMAKE_MODULE_PATH}/BuildSteps/WritePlatformMetadata_linux.cmake
        DEPENDS ${CMAKE_MODULE_PATH}/BuildSteps/WritePlatformMetadata_linux.cmake
        COMMENT "Configuring Linux metainfo"
        )

    # Read the metainfo from the binary directory because it has been configured by Cmake already
    # to have the version string in it
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # All the desktop files are read from the source directory
    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.kicad.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.kicad.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # Translate the MIME info, which are read from the source directory
    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-gerbers.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-gerbers.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-kicad.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    add_custom_target( metadata ALL
                       DEPENDS ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.kicad.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-gerbers.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml
                      )


    # Install icons
    install( DIRECTORY ${PROJECT_SOURCE_DIR}/resources/linux/icons/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/icons
             COMPONENT resources
             )

    # Install MIME type files
    install( DIRECTORY ${PROJECT_BINARY_DIR}/resources/linux/mime/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages
             COMPONENT resources
             FILES_MATCHING
             PATTERN "*.xml"
             PATTERN "*.in" EXCLUDE
             )

    # Install application launchers (.desktop files)
    install( DIRECTORY ${PROJECT_BINARY_DIR}/resources/linux/launchers/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
             COMPONENT resources
             FILES_MATCHING
             PATTERN "*.desktop"
             PATTERN "*.in" EXCLUDE
             )

    # Install appstream metadata (app store entry)
    install( DIRECTORY ${PROJECT_BINARY_DIR}/resources/linux/metainfo/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
             COMPONENT resources
             FILES_MATCHING
             PATTERN "*metainfo.xml"
             PATTERN "*.in" EXCLUDE
             )
endif()
