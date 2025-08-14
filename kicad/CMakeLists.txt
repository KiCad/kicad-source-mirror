# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( KICAD )
add_subdirectory( pcm )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/eeschema
    ${CMAKE_SOURCE_DIR}/kicad/pcm
    ${INC_AFTER}
    )

set( KICAD_SRCS
    dialogs/dialog_destination.cpp
    dialogs/dialog_destination_base.cpp
    dialogs/dialog_copyfiles_job_settings.cpp
    dialogs/dialog_copyfiles_job_settings_base.cpp
    dialogs/dialog_executecommand_job_settings.cpp
    dialogs/dialog_executecommand_job_settings_base.cpp
    dialogs/dialog_update_check_prompt_base.cpp
    dialogs/dialog_update_check_prompt.cpp
    dialogs/dialog_update_notice_base.cpp
    dialogs/dialog_update_notice.cpp
    dialogs/dialog_template_selector_base.cpp
    dialogs/dialog_template_selector.cpp
    dialogs/dialog_edit_cfg.cpp
    dialogs/panel_kicad_launcher_base.cpp
    dialogs/panel_kicad_launcher.cpp
    dialogs/panel_jobset_base.cpp
    dialogs/panel_jobset.cpp
    files-io.cpp
    jobs_runner.cpp
    import_proj.cpp
    import_project.cpp
    kicad_manager_frame.cpp
    menubar.cpp
    project_template.cpp
    project_tree_pane.cpp
    project_tree.cpp
    project_tree_item.cpp
    update_manager.cpp
    toolbars_kicad_manager.cpp
    tools/kicad_manager_actions.cpp
    tools/kicad_manager_control.cpp
    )

set( KICAD_CLI_SRCS
    cli/command.cpp
    cli/command_jobset_run.cpp
    cli/command_pcb_export_base.cpp
    cli/command_pcb_drc.cpp
    cli/command_pcb_render.cpp
    cli/command_pcb_export_3d.cpp
    cli/command_pcb_export_drill.cpp
    cli/command_pcb_export_dxf.cpp
    cli/command_pcb_export_gerber.cpp
    cli/command_pcb_export_gerbers.cpp
    cli/command_pcb_export_gencad.cpp
    cli/command_pcb_export_hpgl.cpp
    cli/command_pcb_export_ipc2581.cpp
    cli/command_pcb_export_ipcd356.cpp
    cli/command_pcb_export_odb.cpp
    cli/command_pcb_export_pdf.cpp
    cli/command_pcb_export_pos.cpp
    cli/command_pcb_export_ps.cpp
    cli/command_pcb_export_svg.cpp
    cli/command_fp_export_svg.cpp
    cli/command_fp_upgrade.cpp
    cli/command_sch_export_bom.cpp
    cli/command_sch_export_pythonbom.cpp
    cli/command_sch_export_netlist.cpp
    cli/command_sch_export_plot.cpp
    cli/command_sch_erc.cpp
    cli/command_sym_export_svg.cpp
    cli/command_sym_upgrade.cpp
    cli/command_version.cpp

    jobs_runner.cpp
    )

if( WIN32 )
    if( MINGW )
        # KICAD_RESOURCES variable is set by the macro.
        mingw_resource_compiler( kicad )
    else()
        set( KICAD_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/kicad.rc )
    endif()
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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME kicad )
endif()

add_executable( kicad WIN32 MACOSX_BUNDLE
    kicad.cpp
    ${KICAD_SRCS}
    ${KICAD_EXTRA_SRCS}
    ${KICAD_RESOURCES}
    )

add_executable( kicad-cli WIN32
    kicad_cli.cpp
    ${KICAD_CLI_SRCS}
    ${KICAD_EXTRA_SRCS}
    ${KICAD_RESOURCES}
    )

if( MSVC )
   # The cli needs subsystem:console or else we can't link wmain/main
   set_target_properties(kicad-cli PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(kicad-cli PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(kicad-cli PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(kicad-cli PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(kicad-cli PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(kicad-cli PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")

   # Allow running from build dir
   add_custom_command( TARGET kicad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kiapi>" "$<TARGET_FILE_DIR:kicad>"
        )
endif()

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
        nlohmann_json
        common
        kicommon
        core
        ${wxWidgets_LIBRARIES}
        )

    set_target_properties( kicad-cli PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
        )

    target_link_libraries( kicad-cli
        nlohmann_json
        common
        ${wxWidgets_LIBRARIES}
        core
        )
else()
    target_link_libraries( kicad
        nlohmann_json
        common
        gal
        common      #repeated due to a circular dependency between gal and common
        kicommon
        core
        ${wxWidgets_LIBRARIES}
        )

    target_link_libraries( kicad-cli
        nlohmann_json
        common
        gal
        common      #repeated due to a circular dependency between gal and common
        kicommon
        core
        ${wxWidgets_LIBRARIES}
        )
endif()

target_link_libraries( kicad pcm )
target_link_libraries( kicad-cli pcm )

install( TARGETS kicad
    DESTINATION ${KICAD_BIN}
    COMPONENT binary
    )

if( NOT APPLE )
    install( TARGETS kicad-cli
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:kicad> DESTINATION ${KICAD_BIN})
endif()

if( APPLE )
    # "install( CODE ... )" will launch its own CMake, so no variables from
    # this CMake instance are accessible... use helper to transfer
    set( SCRIPTING_HELPER "1" )

    set( SPICE_HELPER "1" )

    if( PYTHON_FRAMEWORK )
        set( PYTHON_FRAMEWORK_HELPER "1" )
    else()
        set( PYTHON_FRAMEWORK_HELPER "0" )
    endif()

    set_target_properties( kicad PROPERTIES BUILD_RPATH ${PYTHON_FRAMEWORK} )
    set_target_properties( kicad PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( kicad-cli PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( kicad-cli PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
        set( PYTHON_FRAMEWORK \"${PYTHON_FRAMEWORK}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install the main bundle
        install_runtime_deps( "${OSX_BUNDLE_INSTALL_BIN_DIR}/kicad"
            ""
            ""
            )

        # Now install Python framework properly (GET_RUNTIME_DEPENDENCIES can't do this)
        execute_process( COMMAND rm ${OSX_BUNDLE_INSTALL_LIB_DIR}/Python )
        execute_process( COMMAND cp -RP ${PYTHON_FRAMEWORK} ${OSX_BUNDLE_INSTALL_LIB_DIR}/ )
    ]] )

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
        move_to_main_bundle( \"bitmap2component.app\" \"Image Converter.app\" )
        move_to_main_bundle( \"eeschema.app\" \"Schematic Editor.app\" )
        move_to_main_bundle( \"gerbview.app\" \"GerbView.app\" )
        move_to_main_bundle( \"pcb_calculator.app\" \"PCB Calculator.app\" )
        move_to_main_bundle( \"pcbnew.app\" \"PCB Editor.app\" )
        move_to_main_bundle( \"pl_editor.app\" \"Page Layout Editor.app\" )

        cmake_policy( SET CMP0012 NEW )
        if ( ${PYTHON_FRAMEWORK_HELPER} AND ${KICAD_APPLE_MAKE_RELOCATEABLE_BUNDLE} )
            message( STATUS \"Making the finished bundle relocateable...\" )
            include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/RefixupMacOS.cmake )
            refix_kicad_bundle( ${OSX_BUNDLE_INSTALL_DIR} )
        endif( )

        " COMPONENT Runtime
        )
endif()
