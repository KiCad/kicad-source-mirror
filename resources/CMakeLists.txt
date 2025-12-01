#################################################
# Generate platform metadata files
#################################################
if( APPLE )
    #TODO Generate this at runtime as well
    include( ${KICAD_CMAKE_MODULE_PATH}/WritePlatformMetadata_macos.cmake )
elseif( UNIX )
    # Note: We use two different app IDs here for legacy reasons. The org.kicad.KiCad
    # ID is from FlatHub Flatpaks, while the org.kicad.kicad ID is used everywhere else.
    # Having these two be separate causes problems in Gnome Software because they wll
    # each appear as their own app, so KiCad gets two entries.
    #
    # To work around this, we include a provides statement in the metainfo for the other
    # app ID to link the two IDs together.

    # Default values for regular builds
    set( KICAD_REVERSE_DOMAIN "org.kicad" )
    set( KICAD_APP_NAME "${KICAD_REVERSE_DOMAIN}.kicad" )
    set( KICAD_PROVIDES_APP_ID "${KICAD_REVERSE_DOMAIN}.KiCad" )
    set( KICAD_APP_PREFIX "${KICAD_REVERSE_DOMAIN}" )
    set( KICAD_ICON_PREFIX "" )
    set( KICAD_DESKTOP_FILE_ICON_PREFIX "" )
    set( KICAD_DESKTOP_FILE_ICON_KICAD "kicad" )
    set( KICAD_MIME_FILE_PREFIX "kicad" )
    set( KICAD_MIME_ICON_PREFIX "" )

    # Override default values from above if we are building a flatpak
    if( KICAD_BUILD_FLATPAK )
        set( KICAD_APP_NAME "${KICAD_REVERSE_DOMAIN}.KiCad" )
        set( KICAD_PROVIDES_APP_ID "${KICAD_REVERSE_DOMAIN}.kicad" )
        if( KICAD_BUILD_NIGHTLY_FLATPAK )
            set( KICAD_APP_NAME "${KICAD_APP_NAME}.Nightly" )
            set( KICAD_PROVIDES_APP_ID "${KICAD_PROVIDES_APP_ID}.Nightly" )
        endif()
        set( KICAD_APP_PREFIX "${KICAD_APP_NAME}" )
        set( KICAD_ICON_PREFIX "${KICAD_APP_NAME}" )
        set( KICAD_DESKTOP_FILE_ICON_PREFIX "${KICAD_APP_PREFIX}." )
        set( KICAD_DESKTOP_FILE_ICON_KICAD "${KICAD_APP_NAME}" )
        set( KICAD_MIME_FILE_PREFIX "${KICAD_APP_PREFIX}" )
        set( KICAD_MIME_ICON_PREFIX "${KICAD_APP_PREFIX}." )
    endif()

    # Add the command to configure the linux metainfo file with the version information
    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
        COMMAND ${CMAKE_COMMAND}
            -DSRC_PATH=${PROJECT_SOURCE_DIR}
            -DBUILD_PATH=${PROJECT_BINARY_DIR}
            -DKICAD_CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake
            -DKICAD_APP_NAME=${KICAD_APP_NAME}
            -DKICAD_PROVIDES_APP_ID=${KICAD_PROVIDES_APP_ID}
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WritePlatformMetadata_linux.cmake
        DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/WritePlatformMetadata_linux.cmake
        COMMENT "Configuring Linux metainfo"
        )

    # Read the metainfo from the binary directory because it has been configured by Cmake already
    # to have the version string in it
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/metainfo/${KICAD_APP_NAME}.metainfo.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # All the desktop files take two passes: First they are configured to replace the variables,
    # then they are translated from the binary directory
    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.bitmap2component.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.bitmap2component.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.eeschema.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.eeschema.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.gerbview.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.gerbview.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.kicad.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.kicad.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.kicad.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_NAME}.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbcalculator.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.pcbcalculator.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop.in
                    ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/launchers/org.kicad.pcbnew.desktop.in
                                ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.pcbnew.desktop
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # Translate the MIME info, which are read from the source directory...
    linux_metadata_translation( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-gerbers.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/${KICAD_MIME_FILE_PREFIX}-gerbers.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    # ...except for the KiCad MIME info, which is also configured first to replace variables
    configure_file( ${PROJECT_SOURCE_DIR}/resources/linux/mime/kicad-kicad.xml.in
                    ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml.in
                    @ONLY )
    linux_metadata_translation( ${PROJECT_BINARY_DIR}/resources/linux/mime/kicad-kicad.xml.in
                                ${PROJECT_BINARY_DIR}/resources/linux/mime/${KICAD_MIME_FILE_PREFIX}-kicad.xml
                                ${PROJECT_SOURCE_DIR}/translation/pofiles )

    add_custom_target( metadata ALL
                       DEPENDS ${PROJECT_BINARY_DIR}/resources/linux/metainfo/${KICAD_APP_NAME}.metainfo.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.bitmap2component.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.eeschema.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.gerbview.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_NAME}.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.pcbcalculator.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/launchers/${KICAD_APP_PREFIX}.pcbnew.desktop
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/${KICAD_MIME_FILE_PREFIX}-gerbers.xml
                               ${PROJECT_BINARY_DIR}/resources/linux/mime/${KICAD_MIME_FILE_PREFIX}-kicad.xml
                      )


    # Copy icons to binary dir, where they can be renamed for flatpak, if needed
    file( COPY ${PROJECT_SOURCE_DIR}/resources/linux/icons/
          DESTINATION ${PROJECT_BINARY_DIR}/resources/linux/icons
          FILES_MATCHING
          PATTERN "*.png"
          PATTERN "*.svg"
          )

    if( KICAD_BUILD_FLATPAK )
        # Rename all icons to start with KICAD_ICON_PREFIX
        file( GLOB_RECURSE _ALL_ICONS
              LIST_DIRECTORIES false
              ${PROJECT_BINARY_DIR}/resources/linux/icons/*.png
              ${PROJECT_BINARY_DIR}/resources/linux/icons/*.svg
              )
        foreach( _ICON ${_ALL_ICONS} )
            get_filename_component( _ICON_PATH ${_ICON} DIRECTORY )
            get_filename_component( _ICON_NAME ${_ICON} NAME_WE )
            get_filename_component( _ICON_EXT ${_ICON} EXT )
            if( _ICON_NAME STREQUAL "kicad" )
                set( _RENAMED_ICON "${_ICON_PATH}/${KICAD_APP_NAME}${_ICON_EXT}" )
            else()
                set( _RENAMED_ICON "${_ICON_PATH}/${KICAD_ICON_PREFIX}.${_ICON_NAME}${_ICON_EXT}" )
            endif()
            file( RENAME ${_ICON} ${_RENAMED_ICON} )
        endforeach()
    endif()

    # Install icons from binary dir, as they might have been renamed above
    install( DIRECTORY ${PROJECT_BINARY_DIR}/resources/linux/icons/
             DESTINATION ${CMAKE_INSTALL_DATADIR}/icons
             COMPONENT resources
             FILES_MATCHING
             PATTERN "${KICAD_ICON_PREFIX}*.png"
             PATTERN "${KICAD_ICON_PREFIX}*.svg"
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

    # Install bash completion
    install( FILES ${PROJECT_SOURCE_DIR}/resources/linux/autocomplete/kicad-cli-completion.bash
             DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
             RENAME kicad-cli
             COMPONENT resources
             )

    # Install zsh completion
    install( FILES ${PROJECT_SOURCE_DIR}/resources/linux/autocomplete/_kicad-cli
             DESTINATION ${CMAKE_INSTALL_DATADIR}/zsh/site-functions
             COMPONENT resources
             )
endif()

add_subdirectory(bitmaps_png)
add_subdirectory(project_template)