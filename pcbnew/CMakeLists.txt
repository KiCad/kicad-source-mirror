add_definitions(-DPCBNEW)

###
# Includes
###
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../common
                    ../polygon)

###
# Sources
#
# NOTE: Many of the commented out ones are nested in *.cpp files for dialogs
###
if( USE_BOOST_POLYGON_LIBRARY )
    set(PCBNEW_SRC_FILL_ZONE zones_convert_brd_items_to_polygons_with_Boost.cpp)
else( USE_BOOST_POLYGON_LIBRARY )
    set(PCBNEW_SRC_FILL_ZONE zones_convert_brd_items_to_polygons_with_Kbool.cpp)
endif( USE_BOOST_POLYGON_LIBRARY )

set(PCBNEW_DIALOGS
    dialogs/dialog_block_options_base.cpp
    dialogs/dialog_cleaning_options_base.cpp
    dialogs/dialog_cleaning_options.cpp
    dialogs/dialog_copper_zones.cpp
    dialogs/dialog_copper_zones_base.cpp
    dialogs/dialog_design_rules.cpp
    dialogs/dialog_design_rules_base.cpp
    dialogs/dialog_display_options.cpp
    dialogs/dialog_display_options_base.cpp
    dialogs/dialog_drc_base.cpp
    dialogs/dialog_drc.cpp
    dialogs/dialog_edit_module_for_BoardEditor.cpp
    dialogs/dialog_edit_module_for_BoardEditor_base.cpp
    dialogs/dialog_edit_module_for_Modedit_base.cpp
    dialogs/dialog_edit_module_for_Modedit.cpp
    dialogs/dialog_edit_module_text.cpp
    dialogs/dialog_edit_module_text_base.cpp
    dialogs/dialog_exchange_modules_base.cpp
    dialogs/dialog_export_3Dfiles_base.cpp
    dialogs/dialog_freeroute_exchange.cpp
    dialogs/dialog_freeroute_exchange_base.cpp
    dialogs/dialog_gendrill.cpp
    dialogs/dialog_gendrill_base.cpp
    dialogs/dialog_general_options.cpp
    dialogs/dialog_general_options_BoardEditor_base.cpp
    dialogs/dialog_global_edit_tracks_and_vias.cpp
    dialogs/dialog_global_edit_tracks_and_vias_base.cpp
    dialogs/dialog_global_pads_edition_base.cpp
    dialogs/dialog_graphic_items_options.cpp
    dialogs/dialog_graphic_items_options_base.cpp
    dialogs/dialog_graphic_item_properties.cpp
    dialogs/dialog_graphic_item_properties_base.cpp
    dialogs/dialog_global_deletion.cpp
    dialogs/dialog_global_deletion_base.cpp
    dialogs/dialog_layers_setup.cpp
    dialogs/dialog_layers_setup_base.cpp
    dialogs/dialog_netlist.cpp
    dialogs/dialog_netlist_fbp.cpp
    dialogs/dialog_pcb_text_properties.cpp
    dialogs/dialog_pcb_text_properties_base.cpp
    dialogs/dialog_non_copper_zones_properties_base.cpp
    dialogs/dialog_pad_properties.cpp
    dialogs/dialog_pad_properties_base.cpp
    dialogs/dialog_pcbnew_config_libs_and_paths.cpp
    dialogs/dialog_pcbnew_config_libs_and_paths_fbp.cpp
    dialogs/dialog_plot_base.cpp
    dialogs/dialog_print_for_modedit.cpp
    dialogs/dialog_print_for_modedit_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_orient_footprints.cpp
    dialogs/dialog_orient_footprints_base.cpp
    dialogs/dialog_mask_clearance.cpp
    dialogs/dialog_mask_clearance_base.cpp
    dialogs/dialog_SVG_print.cpp
    dialogs/dialog_SVG_print_base.cpp
    dialogs/dialog_set_grid_base.cpp
    )

set(PCBNEW_SRCS
    pcbframe.cpp
    attribut.cpp
    automove.cpp
    autoplac.cpp
    autorout.cpp
    board.cpp
    board_items_to_polygon_shape_transform.cpp
    board_undo_redo.cpp
    block.cpp
    block_module_editor.cpp
    build_BOM_from_board.cpp
    class_pcb_layer_widget.cpp
    clean.cpp
    connect.cpp
    controle.cpp
    dimension.cpp
    cross-probing.cpp
    debug_kbool_key_file_fct.cpp
    deltrack.cpp
    ${PCBNEW_DIALOGS}
    dist.cpp
    dragsegm.cpp
    drc.cpp
    drc_clearance_test_functions.cpp
    drc_marker_functions.cpp
    edgemod.cpp
    edit.cpp
    editedge.cpp
    editmod.cpp
    editrack.cpp
    editrack-part2.cpp
    edit_pcb_text.cpp
    edit_track_width.cpp
    edtxtmod.cpp
    event_handlers_tracks_vias_sizes.cpp
    export_gencad.cpp
    export_vrml.cpp
    files.cpp
    find.cpp
    gen_drill_report_files.cpp
    gen_holes_and_tools_lists_for_drill.cpp
    gen_modules_placefile.cpp
    gendrill.cpp
    globaleditpad.cpp
    gpcb_exchange.cpp
    graphpcb.cpp
    hotkeys.cpp
    hotkeys_board_editor.cpp
    hotkeys_module_editor.cpp
    initpcb.cpp
    ioascii.cpp
    layer_widget.cpp
    librairi.cpp
    loadcmp.cpp
    locate.cpp
    magnetic_tracks_functions.cpp
    menubar_modedit.cpp
    menubar_pcbframe.cpp
    mirepcb.cpp
    modedit.cpp
    modedit_onclick.cpp
    modeditoptions.cpp
    modedit_undo_redo.cpp
    moduleframe.cpp
    modules.cpp
    move-drag_pads.cpp
    move_or_drag_track.cpp
    muonde.cpp
    muwave_command.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    pcbnew.cpp
    pcbnew_config.cpp
    pcbplot.cpp
    plotgerb.cpp
    plothpgl.cpp
    plotps.cpp
    plotdxf.cpp
    plot_rtn.cpp
    print_board_functions.cpp
    printout_controler.cpp
    queue.cpp
    ratsnest.cpp
    set_grid.cpp
    solve.cpp
    specctra.cpp
    specctra_export.cpp
    specctra_import.cpp
    surbrill.cpp
    swap_layers.cpp
    tool_modedit.cpp
    tool_onrightclick.cpp
    tool_pcb.cpp
    toolbars_update_user_interface.cpp
    tracepcb.cpp
    track.cpp
    tr_modif.cpp
    trpiste.cpp
    work.cpp
    xchgmod.cpp
    ${PCBNEW_SRC_FILL_ZONE}
    zones_convert_to_polygons_aux_functions.cpp
    zones_by_polygon.cpp
    zones_by_polygon_fill_functions.cpp
    zone_filling_algorithm.cpp
    zones_functions_for_undo_redo.cpp
    zones_non_copper_type_functions.cpp
    zones_polygons_insulated_copper_islands.cpp
    zones_polygons_test_connections.cpp
    zones_test_and_combine_areas.cpp
    )

###
# We need some extra sources from common
###
set(PCBNEW_EXTRA_SRCS
    ../common/dialogs/dialog_page_settings.cpp
)

###
# Windows resource file
###
if(WIN32)
    if(MINGW)
        # PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler(pcbnew)
    else(MINGW)
        set(PCBNEW_RESOURCES pcbnew.rc)
    endif(MINGW)
endif(WIN32)

###
# Set properties for APPLE on pcbnew target
###
if(APPLE)
    set(PCBNEW_RESOURCES pcbnew.icns pcbnew_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcbnew_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE pcbnew.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcbnew)
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

# Create a C++ compilable string initializer containing html text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dialog_freeroute_exchange_help_html.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
        -P ${CMAKE_MODULE_PATH}/Html2C.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html"
    )

set_source_files_properties( dialogs/dialog_freeroute_exchange.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
    )

###
# Create the pcbnew executable
###
add_executable(pcbnew WIN32 MACOSX_BUNDLE
               ${PCBNEW_SRCS}
               ${PCBNEW_EXTRA_SRCS}
               ${PCBNEW_RESOURCES}
              )

###
# Set properties for APPLE on pcbnew target
###
if(APPLE)
    set_target_properties(pcbnew PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

###
# Link executable target pcbnew with correct libraries
###
target_link_libraries(pcbnew 3d-viewer common pcbcommon polygon bitmaps kbool
                      ${OPENGL_LIBRARIES}
                      ${wxWidgets_LIBRARIES}
                      ${GDI_PLUS_LIBRARIES})

###
# Add pcbnew as install target
###
install(TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)


# The specctra test fails to build properly using MS Visual Studio.
if(NOT MSVC)
    # This one gets made only when testing.
    add_executable(specctra_test EXCLUDE_FROM_ALL specctra_test.cpp specctra.cpp)
    target_link_libraries(specctra_test common ${wxWidgets_LIBRARIES})
endif(NOT MSVC)

# This one gets made only when testing.
add_executable(layer_widget_test WIN32 EXCLUDE_FROM_ALL
    layer_widget.cpp
    )
target_link_libraries(layer_widget_test common ${wxWidgets_LIBRARIES})
