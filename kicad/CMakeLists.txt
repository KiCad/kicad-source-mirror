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
    # In this CMakeLists.txt's build directory, create kiface symlinks should get
    # "installed()" as part of the kicad.app bundle.  These are pointers on the
    # target which point up and over to the stand alone kicad app's *.kiface files.
    foreach( symlink pcbnew eeschema cvpcb )
        add_custom_command( TARGET kicad
            COMMAND ${CMAKE_COMMAND} -E remove
                        "${CMAKE_CURRENT_BINARY_DIR}/kicad.app/Contents/MacOS/_${symlink}.kiface"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                        "../../../${symlink}.app/Contents/MacOS/_${symlink}.kiface"
                        "${CMAKE_CURRENT_BINARY_DIR}/kicad.app/Contents/MacOS/_${symlink}.kiface"
            COMMENT "kicad.app ${symlink} symlink"
            )
    endforeach()
endif()

if( APPLE )
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

