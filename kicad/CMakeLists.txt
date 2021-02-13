# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions( -DKICAD )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/eeschema
    ${INC_AFTER}
    )


set( KICAD_SRCS
    dialogs/dialog_template_selector_base.cpp
    dialogs/dialog_template_selector.cpp
    dialogs/panel_kicad_launcher_base.cpp
    dialogs/panel_kicad_launcher.cpp
    files-io.cpp
    import_project.cpp
    kicad.cpp
    kicad_manager_frame.cpp
    kicad_settings.cpp
    menubar.cpp
    project_template.cpp
    project_tree_pane.cpp
    project_tree.cpp
    project_tree_item.cpp
    tools/kicad_manager_actions.cpp
    tools/kicad_manager_control.cpp
    )

if( MINGW )
    # KICAD_RESOURCES variable is set by the macro.
    mingw_resource_compiler( kicad )
else()
    set( KICAD_RESOURCES kicad.rc )
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

target_include_directories( kicad PRIVATE
        $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
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
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/kicad/Info.plist
        )
    target_link_libraries( kicad
        common
        ${wxWidgets_LIBRARIES}
        )
else()
    target_link_libraries( kicad
        common
        gal
        common      #repeated due to a circular dependancy bewteen gal and common
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

    if( KICAD_SPICE )
        set( SPICE_HELPER "1" )
    else()
        set( SPICE_HELPER "0" )
    endif()

    if( KICAD_USE_OCC )
        set( KICAD_BUNDLE_LIBS ${OCC_LIBRARY_DIR} )
    endif()

    # make main bundle relocatable
    install( CODE "
        # find all kicad libs and modules
        file( GLOB PLUGINS_KIFACE ${OSX_BUNDLE_INSTALL_KIFACE_DIR}/*.kiface )
        file( GLOB_RECURSE PLUGINS_SO ${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/*.so )
        file( GLOB_RECURSE PLUGINS_3D ${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/3d/*.dylib )
        set( BUNDLE_FIX_LIBS \${PLUGINS_KIFACE} \${PLUGINS_SO} \${PLUGINS_3D} )

        # Find ngspice if it is being packaged
        if( ${SPICE_HELPER} )
            file( GLOB_RECURSE PLUGINS_NGSPICE ${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim/*.dylib )
            set( BUNDLE_FIX_LIBS \${BUNDLE_FIX_LIBS} \${PLUGINS_NGSPICE} )
        endif()

        # Find python if it is requested
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

        fixup_bundle( ${OSX_BUNDLE_INSTALL_BIN_DIR}/kicad
            \"\${BUNDLE_FIX_LIBS}\"
            \" ${KICAD_BUNDLE_LIBS}\"
            )

        if( ${SPICE_HELPER} )
            execute_process( COMMAND install_name_tool -id @executable_path/../PlugIns/sim/libngspice.0.dylib libngspice.0.dylib
                             WORKING_DIRECTORY ${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim )
        endif()

        " COMPONENT Runtime
        )

    # move all individual app bundles into main bundle
    install( CODE "
        # helper function to move a bundle into main bundle
        function( move_to_main_bundle bundle_name symlink_name )
            message( STATUS \"Moving \${bundle_name} into main bundle...\" )
            file( MAKE_DIRECTORY ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR} )
            file( REMOVE_RECURSE ${KICAD_BIN}/\${bundle_name}/${OSX_BUNDLE_LIB_DIR} )
            file( RENAME ${KICAD_BIN}/\${bundle_name} ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR}/\${bundle_name} )
            execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \"../../../Frameworks\" \"Frameworks\"
                WORKING_DIRECTORY ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_APP_DIR}/\${bundle_name}/Contents
            )
            # create a top-level link pointing inside main bundle
            execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink \"${OSX_BUNDLE_MAIN}/${OSX_BUNDLE_APP_DIR}/\${bundle_name}\" \"\${symlink_name}\"
                WORKING_DIRECTORY ${KICAD_BIN}
            )
        endfunction( move_to_main_bundle )

        # move all app bundles
        move_to_main_bundle( \"bitmap2component.app\" \"Bitmap2Component.app\" )
        move_to_main_bundle( \"eeschema.app\" \"Eeschema.app\" )
        move_to_main_bundle( \"gerbview.app\" \"GerbView.app\" )
        move_to_main_bundle( \"pcb_calculator.app\" \"PCB Calculator.app\" )
        move_to_main_bundle( \"pcbnew.app\" \"Pcbnew.app\" )
        move_to_main_bundle( \"pl_editor.app\" \"Page Layout Editor.app\" )
        " COMPONENT Runtime
        )
endif()
