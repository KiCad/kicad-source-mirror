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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
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
if( APPLE )
    # "install( CODE ... )" will launch its own CMake, so no variables from
    # this CMake instance are accessible... use helper to transfer
    if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )
        set( SCRIPTING_HELPER "1" )
    else()
        set( SCRIPTING_HELPER "0" )
    endif()

    # make main bundle relocatable
    install( CODE "
        # find all libs and modules
        file( GLOB BUNDLE_FIX_LIBS ${OSX_BUNDLE_INSTALL_KIFACE_DIR}/*.kiface )
        if( ${SCRIPTING_HELPER} )
            file( GLOB WXPYTHON_DIR RELATIVE ${OSX_BUNDLE_INSTALL_LIB_DIR}/${PYTHON_LIB_DIR} ${OSX_BUNDLE_INSTALL_LIB_DIR}/${PYTHON_LIB_DIR}/wx-?.?-osx_cocoa )
            file( GLOB PYTHON_SCRIPTING_SO ${OSX_BUNDLE_INSTALL_LIB_DIR}/${PYTHON_LIB_DIR}/*.so )
            set( BUNDLE_FIX_LIBS \${BUNDLE_FIX_LIBS} \${PYTHON_SCRIPTING_SO} )
            file( GLOB PYTHON_SCRIPTING_SO ${OSX_BUNDLE_INSTALL_LIB_DIR}/${PYTHON_LIB_DIR}/\${WXPYTHON_DIR}/wx/*.so )
            set( BUNDLE_FIX_LIBS \${BUNDLE_FIX_LIBS} \${PYTHON_SCRIPTING_SO} )
        endif()

        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )

        # If `BU_CHMOD_BUNDLE_ITEMS` is not set, `install_name_tool` will fail to re-write some loader paths due to lack of writable permissions if the build dependencies were installed by brew (or didn't have writable permissions)
        set(BU_CHMOD_BUNDLE_ITEMS ON)

        fixup_bundle( ${OSX_BUNDLE_INSTALL_BIN_DIR}/kicad
            \"\${BUNDLE_FIX_LIBS}\"
            \"\"
            )
        " COMPONENT Runtime
        )

    # move all individual app bundles into main bundle
    install( CODE "
        # helper function to move a bundle into main bundle
        function( move_to_main_bundle bundle_name )
            message( STATUS \"Moving \${bundle_name} into main bundle...\" )
            file( MAKE_DIRECTORY ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR} )
            file( REMOVE_RECURSE ${KICAD_BIN}/\${bundle_name}/${OSX_BUNDLE_LIB_DIR} )
            file( RENAME ${KICAD_BIN}/\${bundle_name} ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR}/\${bundle_name} )
            execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \"../../../Frameworks\" \"Frameworks\"
                WORKING_DIRECTORY ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR}/\${bundle_name}/Contents
            )
            # create a top-level link pointing inside main bundle
            execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \"${OSX_BUNDLE_MAIN}/${OSX_BUNDLE_APP_DIR}/\${bundle_name}\" \"\${bundle_name}\"
                WORKING_DIRECTORY ${KICAD_BIN}
            )
        endfunction( move_to_main_bundle )

        # move all app bundles
        move_to_main_bundle( bitmap2component.app )
        move_to_main_bundle( eeschema.app )
        move_to_main_bundle( gerbview.app )
        move_to_main_bundle( pcb_calculator.app )
        move_to_main_bundle( pcbnew.app )
        move_to_main_bundle( pl_editor.app )
        " COMPONENT Runtime
        )
endif()
