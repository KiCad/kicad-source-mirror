#################################################
# Generate platform metadata files
#################################################
if( APPLE )
    #TODO Generate this at runtime as well
    include( ${CMAKE_MODULE_PATH}/WritePlatformMetadata_macos.cmake )
elseif( UNIX )
    # Add the command to configure the linux appdata file with the version information
    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/resources/linux/appdata/kicad.appdata.xml.in
        COMMAND ${CMAKE_COMMAND}
            -DSRC_PATH=${PROJECT_SOURCE_DIR}
            -DBUILD_PATH=${PROJECT_BINARY_DIR}
            -DCMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/CMakeModules
            -P ${CMAKE_MODULE_PATH}/WritePlatformMetadata_linux.cmake
        COMMENT "Configuring Linux appdata"
        )

    # Read the appdata from the binary directory because it has been configured by Cmake already
    # to have the version string in it
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/appdata/kicad.appdata.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/appdata/kicad.appdata.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # All the desktop files are read from the source directory
    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/bitmap2component.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/bitmap2component.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/eeschema.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/eeschema.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/gerbview.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/gerbview.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/kicad.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/kicad.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/pcbcalculator.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/pcbcalculator.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/pcbnew.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/pcbnew.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # Translate the MIME info, which are read from the source directory
    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-gerbers.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-gerbers.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-kicad.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    add_custom_target( metadata ALL
                       DEPENDS ${PROJECT_BINARY_DIR}/resources/linux/appdata/kicad.appdata.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/bitmap2component.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/eeschema.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/gerbview.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/kicad.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/pcbcalculator.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/pcbnew.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-gerbers.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml
                      )


    # Install icons
    install( DIRECTORY ${PROJECT_SOURCE_DIR}/resources/linux/icons
             TYPE DATA
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
    install( DIRECTORY ${PROJECT_BINARY_DIR}/resources/linux/appdata/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata
             COMPONENT resources
             FILES_MATCHING
             PATTERN "*appdata.xml"
             PATTERN "*.in" EXCLUDE
             )
endif()
