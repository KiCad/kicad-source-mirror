add_definitions( -DKICAD )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${INC_AFTER}
    )


set( KICAD_SRCS
    class_treeprojectfiles.cpp
    class_treeproject_item.cpp
    commandframe.cpp
    dialogs/dialog_template_selector_base.cpp
    dialogs/dialog_template_selector.cpp
    files-io.cpp
    kicad.cpp
    mainframe.cpp
    menubar.cpp
    preferences.cpp
    prjconfig.cpp
    project_template.cpp
    tree_project_frame.cpp
    )

if( MINGW )
    # KICAD_RESOURCES variable is set by the macro.
    mingw_resource_compiler( kicad )
endif()

if( APPLE )
    set( KICAD_RESOURCES kicad.icns kicad_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/kicad.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/kicad_doc.icns"  PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE kicad.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME kicad )
endif()

add_executable( kicad WIN32 MACOSX_BUNDLE
    ${KICAD_SRCS}
    ${KICAD_EXTRA_SRCS}
    ${KICAD_RESOURCES}
    )

if( UNIX )
    # for build directory: create kiface symlinks so kicad (exe) can be run in-situ
    add_custom_target( kiface_sym_links
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/eeschema/_eeschema.kiface" "${CMAKE_BINARY_DIR}/kicad/_eeschema.kiface"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.kiface" "${CMAKE_BINARY_DIR}/kicad/_pcbnew.kiface"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/cvpcb/_cvpcb.kiface" "${CMAKE_BINARY_DIR}/kicad/_cvpcb.kiface"
        COMMENT "Making <build-dir>/kicad/<kiface.symlinks>"
        )
endif()


if( APPLE )
    # binaries have to be build and kiface be in place
    # Mockup the destination directory to linking symbolical
    # Could be usefully also for debug in building place
    add_custom_target( kiway-kicad-bundle-applinks ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/eeschema/eeschema.app" "${CMAKE_BINARY_DIR}/kicad/eeschema.app"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/pcbnew/pcbnew.app" "${CMAKE_BINARY_DIR}/kicad/pcbnew.app"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/cvpcb/cvpcb.app" "${CMAKE_BINARY_DIR}/kicad/cvpcb.app"
        DEPENDS eeschema pcbnew cvpcb eeschema_kiface pcbnew_kiface cvpcb_kiface
        COMMENT "Making bundles links in kicad.app directory"
    )

    # Kicad is dependent by other applications (otherwise what we could symlink?)
    add_dependencies( kicad kiway-kicad-bundle-applinks ) 

    # making kiface links relative - .app symlinks are alredy in place !
    add_custom_target( kiway-kicad-bundle-kiface-links ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink "../../../eeschema.app/Contents/MacOS/_eeschema.kiface" "_eeschema.kiface"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "../../../pcbnew.app/Contents/MacOS/_pcbnew.kiface" "_pcbnew.kiface"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "../../../cvpcb.app/Contents/MacOS/_cvpcb.kiface" "_cvpcb.kiface"
        DEPENDS kicad
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/kicad/kicad.app/Contents/MacOS"
        COMMENT "Making Symbolik link to kiface libs in Kicad.app Bundle"
    )

    set_target_properties( kicad PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )


    target_link_libraries( kicad
        common
        bitmaps
        polygon
        ${wxWidgets_LIBRARIES}
        )
else()
    target_link_libraries( kicad
        common
        bitmaps
        polygon
        ${wxWidgets_LIBRARIES}
        ${GDI_PLUS_LIBRARIES}
        )
endif()

install( TARGETS kicad
    DESTINATION ${KICAD_BIN}
    COMPONENT binary
    )

