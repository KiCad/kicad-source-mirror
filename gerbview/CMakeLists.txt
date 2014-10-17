# the map generation creates on Windows/gcc a lot of useless warnings
# so disable it on windows
if( WIN32 AND NOT CMAKE_CROSSCOMPILING )
    set( MAKE_LINK_MAPS false )
else()
    set( MAKE_LINK_MAPS true )
endif()

add_definitions(-DGERBVIEW)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ../pcbnew
    dialogs
    ../3d-viewer
    ../polygon
    ${INC_AFTER}
    )


set( DIALOGS_SRCS
    dialogs/gerbview_dialog_display_options_frame_base.cpp
    dialogs/gerbview_dialog_display_options_frame.cpp
    dialogs/dialog_layers_select_to_pcb_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_select_one_pcb_layer.cpp
    dialogs/dialog_show_page_borders.cpp
    dialogs/dialog_show_page_borders_base.cpp
    )

set( GERBVIEW_SRCS
    block.cpp
    class_am_param.cpp
    class_aperture_macro.cpp
    class_DCodeSelectionbox.cpp
    class_gbr_screen.cpp
    class_gbr_layout.cpp
    class_GERBER.cpp
    class_gerber_draw_item.cpp
    class_gerbview_layer_widget.cpp
    class_gbr_layer_box_selector.cpp
    controle.cpp
    dcode.cpp
    draw_gerber_screen.cpp
    events_called_functions.cpp
    excellon_read_drill_file.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerbview_config.cpp
    gerbview_frame.cpp
    hotkeys.cpp
    init_gbr_drawlayers.cpp
    locate.cpp
    menubar.cpp
    onleftclick.cpp
    onrightclick.cpp
    pcbplot.cpp
    printout_control.cpp
    readgerb.cpp
    rs274_read_XY_and_IJ_coordinates.cpp
    rs274d.cpp
    rs274x.cpp
    select_layers_to_pcb.cpp
    toolbars_gerber.cpp
    )

set( GERBVIEW_EXTRA_SRCS
    ../common/base_screen.cpp
    ../common/base_units.cpp
    ../common/eda_text.cpp
    ../common/class_layer_box_selector.cpp
    ../common/class_page_info.cpp
    ../pcbnew/layer_widget.cpp
)

if( MINGW )
    # GERBVIEW_RESOURCES variable is set by the macro.
    mingw_resource_compiler( gerbview )
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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME gerbview )
endif()

if( USE_KIWAY_DLLS )

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
        bitmaps
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
        polygon
        bitmaps
        ${OPENGL_LIBRARIES}
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
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
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

else()

    add_executable( gerbview WIN32 MACOSX_BUNDLE
        gerbview.cpp
        ${GERBVIEW_SRCS}
        ${DIALOGS_SRCS}
        ${GERBVIEW_EXTRA_SRCS}
        ${GERBVIEW_RESOURCES}
        )
    target_link_libraries( gerbview
        common
        polygon
        bitmaps
        ${OPENGL_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${GDI_PLUS_LIBRARIES}
        )
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

endif()
