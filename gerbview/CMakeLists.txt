# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

add_definitions(-DGERBVIEW)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ../include/legacy_wx
    ../pcbnew
    dialogs
    ../common
    ../3d-viewer
    ${INC_AFTER}
    )


set( DIALOGS_SRCS
    dialogs/panel_gerbview_display_options.cpp
    dialogs/panel_gerbview_display_options_base.cpp
    dialogs/panel_gerbview_settings.cpp
    dialogs/panel_gerbview_settings_base.cpp
    dialogs/dialog_layers_select_to_pcb_base.cpp
    dialogs/dialog_print_gerbview.cpp
    dialogs/dialog_select_one_pcb_layer.cpp
    )

set( GERBVIEW_SRCS
    block.cpp
    am_param.cpp
    am_primitive.cpp
    DCodeSelectionbox.cpp
    gbr_screen.cpp
    gbr_layout.cpp
    gerber_file_image.cpp
    gerber_file_image_list.cpp
    gerber_draw_item.cpp
    gerbview_layer_widget.cpp
    gerbview_printout.cpp
    gbr_layer_box_selector.cpp
    X2_gerber_attributes.cpp
    clear_gbr_drawlayers.cpp
    controle.cpp
    dcode.cpp
    draw_gerber_screen.cpp
    evaluate.cpp
    events_called_functions.cpp
    excellon_read_drill_file.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerbview_config.cpp
    gerbview_frame.cpp
    hotkeys.cpp
    json11.cpp
    job_file_reader.cpp
    locate.cpp
    menubar.cpp
    onleftclick.cpp
    onrightclick.cpp
    readgerb.cpp
    rs274_read_XY_and_IJ_coordinates.cpp
    rs274d.cpp
    rs274x.cpp
    select_layers_to_pcb.cpp
    toolbars_gerber.cpp

    gerbview_draw_panel_gal.cpp
    gerbview_painter.cpp

    tools/gerbview_actions.cpp
    tools/selection_tool.cpp
    tools/gerbview_control.cpp
    gerber_collectors.cpp
    )

set( GERBVIEW_EXTRA_SRCS
    ../common/base_screen.cpp
    ../common/base_units.cpp
    ../common/eda_text.cpp
    ../common/widgets/layer_box_selector.cpp
    ../common/lset.cpp
    ../common/settings.cpp
    ../pcbnew/layer_widget.cpp
    ../common/page_info.cpp
)

if( MINGW )
    # GERBVIEW_RESOURCES variable is set by the macro.
    mingw_resource_compiler( gerbview )
else()
    set( GERBVIEW_RESOURCES gerbview.rc )
endif()

if( APPLE )
    # setup bundle
    set( GERBVIEW_RESOURCES gerbview.icns gerbview_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/gerbview.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/gerbview_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE gerbview.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME gerbview )
endif()

add_executable( gerbview WIN32 MACOSX_BUNDLE
    ../common/single_top.cpp
    ../common/pgm_base.cpp
    ${GERBVIEW_RESOURCES}
    )
set_source_files_properties( ../common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_GERBER;BUILD_KIWAY_DLL"
    )
target_link_libraries( gerbview
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    common
    gal
    legacy_wx
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( gerbview PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=gerbview.map" )
endif()

# the main gerbview program, in DSO form.
add_library( gerbview_kiface MODULE
    gerbview.cpp
    ${GERBVIEW_SRCS}
    ${DIALOGS_SRCS}
    ${GERBVIEW_EXTRA_SRCS}
    )
set_target_properties( gerbview_kiface PROPERTIES
    OUTPUT_NAME     gerbview
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )
target_link_libraries( gerbview_kiface
    common
    gal
    legacy_wx
    common
    gal
    legacy_wx
    common
    gal
    legacy_wx
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )
set_source_files_properties( gerbview.cpp PROPERTIES
    # The KIFACE is in gerbview.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( MAKE_LINK_MAPS )
    set_target_properties( gerbview_kiface PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=_gerbview.kiface.map" )
endif()

# if building gerbview, then also build gerbview_kiface if out of date.
add_dependencies( gerbview gerbview_kiface )

# these 2 binaries are a matched set, keep them together
if( APPLE )
    set_target_properties( gerbview PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/gerbview/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( gerbview_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
            # override default embedded path settings
            ${OSX_BUNDLE_OVERRIDE_PATHS}

            # do all the work
            include( BundleUtilities )
            fixup_bundle( ${KICAD_BIN}/gerbview.app/Contents/MacOS/gerbview
                \"\"
                \"\"
                )
            " COMPONENT Runtime
        )
else()
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS gerbview_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()
