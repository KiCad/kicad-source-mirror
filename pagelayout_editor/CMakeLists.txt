# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions(-DPL_EDITOR)

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
    ${CMAKE_SOURCE_DIR}/common/base_screen.cpp
    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    ${CMAKE_SOURCE_DIR}/common/eda_text.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_gal_display_options.cpp
    ${CMAKE_SOURCE_DIR}/common/page_info.cpp
)


if( MINGW )
    # PL_EDITOR_RESOURCES variable is set by the macro.
    mingw_resource_compiler( pl_editor )
else()
    set( PL_EDITOR_RESOURCES pl_editor.rc )
endif()


if( APPLE )
    # setup bundle
    set( PL_EDITOR_RESOURCES pl_editor.icns pl_editor_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pl_editor.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pl_editor_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pl_editor.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME pl_editor )
endif()

# a very small program launcher for pl_editor_kiface
add_executable( pl_editor WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_base.cpp
    ${PL_EDITOR_RESOURCES}
    )
set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_PL_EDITOR;PGM_DATA_FILE_EXT=\"kicad_wks\";BUILD_KIWAY_DLL"
    )
target_link_libraries( pl_editor
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    gal
    common
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( pl_editor PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=pl_editor.map" )
endif()

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
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
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

if( MAKE_LINK_MAPS )
    set_target_properties( pl_editor_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_pl_editor.kiface.map" )
endif()

# if building pl_editor, then also build pl_editor_kiface if out of date.
add_dependencies( pl_editor pl_editor_kiface )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( pl_editor PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/pagelayout_editor/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( pl_editor_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS pl_editor
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )
        fixup_bundle( ${KICAD_BIN}/pl_editor.app/Contents/MacOS/pl_editor
            \"\"
            \"\"
            )
        " COMPONENT Runtime
        )
else()
    install( TARGETS pl_editor
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pl_editor_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()
