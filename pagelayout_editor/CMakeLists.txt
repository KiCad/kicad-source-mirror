# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( PL_EDITOR )

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    dialogs
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${INC_AFTER}
    )

set( DIALOGS_SRCS
    dialogs/properties_frame.cpp
    dialogs/properties_frame_base.cpp
    dialogs/dialogs_for_printing.cpp
    dialogs/dialog_new_dataitem_base.cpp
    dialogs/dialog_design_inspector_base.cpp
    dialogs/design_inspector.cpp
    dialogs/panel_pl_editor_color_settings.cpp
    dialogs/panel_pl_editor_color_settings_base.cpp
    dialogs/panel_pl_editor_display_options.cpp
    )

set( PL_EDITOR_SRCS
    pl_editor_layout.cpp
    files.cpp
    pl_editor_frame.cpp
    pl_editor_settings.cpp
    pl_draw_panel_gal.cpp
    pl_editor_undo_redo.cpp
    menubar.cpp
    toolbars_pl_editor.cpp

    tools/pl_actions.cpp
    tools/pl_selection.cpp
    tools/pl_selection_tool.cpp
    tools/pl_drawing_tools.cpp
    tools/pl_edit_tool.cpp
    tools/pl_editor_control.cpp
    tools/pl_point_editor.cpp
    )

set( PL_EDITOR_EXTRA_SRCS
)

if( WIN32 )
    if( MINGW )
        # PL_EDITOR_RESOURCES variable is set by the macro.
        mingw_resource_compiler( pl_editor )
    else()
        set( PL_EDITOR_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/pl_editor.rc )
    endif()
endif()


if( APPLE )
    # setup bundle
    set( PL_EDITOR_RESOURCES pagelayout_editor.icns pagelayout_editor_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pagelayout_editor.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pagelayout_editor_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pagelayout_editor.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME pl_editor )
endif()

# a very small program launcher for pl_editor_kiface
add_executable( pl_editor WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${PL_EDITOR_RESOURCES}
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_PL_EDITOR;PGM_DATA_FILE_EXT=\"kicad_wks\";BUILD_KIWAY_DLL"
    )
target_link_libraries( pl_editor
    kicommon
    ${wxWidgets_LIBRARIES}
    )

target_link_options( pl_editor PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=pl_editor.map>
    )

# the main pl_editor program, in DSO form.
add_library( pl_editor_kiface MODULE
    pl_editor.cpp
    ${PL_EDITOR_SRCS}
    ${DIALOGS_SRCS}
    ${PL_EDITOR_EXTRA_SRCS}
    )
target_link_libraries( pl_editor_kiface
    gal
    common
    core
    ${wxWidgets_LIBRARIES}
    )
set_target_properties( pl_editor_kiface PROPERTIES
    OUTPUT_NAME     pl_editor
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

set_source_files_properties( pl_editor.cpp PROPERTIES
    # The KIFACE is in pcbnew.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

target_link_options( pl_editor_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=_pl_editor.kiface.map>
    )

# if building pl_editor, then also build pl_editor_kiface if out of date.
add_dependencies( pl_editor pl_editor_kiface )

message( STATUS "Including 3Dconnexion SpaceMouse navigation support in pagelayout editor" )
add_subdirectory( navlib )

target_link_libraries( pl_editor_kiface pl_editor_navlib)

add_dependencies( pl_editor_kiface pl_editor_navlib)

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( pl_editor PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/pagelayout_editor/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( pl_editor_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )

    set_target_properties( pl_editor PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )

    set_target_properties( pl_editor_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( pl_editor_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS pl_editor
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( KICAD_BIN \"${KICAD_BIN}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install any dependencies (this will generally duplicate kicad.app but we can't be sure)
        install_runtime_deps( "${KICAD_BIN}/pl_editor.app/Contents/MacOS/pl_editor"
            ""
            ""
            )
    ]] )
else()
    if( MSVC )
        target_sources( pl_editor_kiface PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/pl_editor-dll.rc )
    endif()

    install( TARGETS pl_editor
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pl_editor_kiface
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:pl_editor> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:pl_editor_kiface> DESTINATION ${KICAD_KIFACE})
endif()
