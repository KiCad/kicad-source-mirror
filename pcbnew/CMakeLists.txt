add_definitions(-DPCBNEW)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../polygon)

# Many of the commented out ones are nested in *.cpp files for dialogs
set(PCBNEW_SRCS
    attribut.cpp
    automove.cpp
    autoplac.cpp
    autorout.cpp
    block.cpp
    block_module_editor.cpp
    board.cpp
    board_undo_redo.cpp
    build_BOM_from_board.cpp
    clean.cpp
#   cleaningoptions_dialog.cpp
    connect.cpp
    controle.cpp
#   copy_track.cpp <-- not used
    cotation.cpp
    cross-probing.cpp
    debug_kbool_key_file_fct.cpp
    deltrack.cpp
    dialog_copper_layers_setup_base.cpp
    dialog_copper_layers_setup.cpp
    dialog_copper_zones.cpp
    dialog_copper_zones_base.cpp
    dialog_design_rules.cpp
    dialog_design_rules_base.cpp
    dialog_display_options.cpp
    dialog_display_options_base.cpp
    dialog_drc_base.cpp
    dialog_drc.cpp
    dialog_edit_module_for_BoardEditor.cpp
    dialog_edit_module_for_BoardEditor_base.cpp
    dialog_edit_module_for_Modedit_base.cpp
    dialog_edit_module_for_Modedit.cpp
    dialog_edit_module_text.cpp
    dialog_edit_module_text_base.cpp
    dialog_exchange_modules_base.cpp
    dialog_freeroute_exchange.cpp
#   dialog_gendrill.cpp
    dialog_general_options.cpp
    dialog_general_options_BoardEditor_base.cpp
#   dialog_graphic_items_options.cpp
    dialog_graphic_item_properties.cpp
    dialog_graphic_item_properties_base.cpp
#   dialog_initpcb.cpp
    dialog_netlist.cpp
    dialog_netlist_fbp.cpp
    dialog_pcb_text_properties.cpp
    zones_non_copper_type_functions.cpp
    dialog_non_copper_zones_properties_base.cpp
    dialog_pad_properties.cpp
    dialog_pad_properties_base.cpp
    dialog_pcbnew_config_libs_and_paths.cpp
    dialog_pcbnew_config_libs_and_paths_fbp.cpp
    dialog_print_using_printer.cpp
    dialog_orient_footprints.cpp
    dialog_track_options.cpp
    dialog_track_options_base.cpp
    dialog_SVG_print.cpp
    dialog_SVG_print_base.cpp
    dist.cpp
    dragsegm.cpp
    drc.cpp
    dsn.cpp
#   dupltrac.cpp <-- not used
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
    initpcb.cpp
    ioascii.cpp
    print_board_functions.cpp
    librairi.cpp
    loadcmp.cpp
    locate.cpp
    magnetic_tracks_functions.cpp
    menubarmodedit.cpp
    menubarpcb.cpp
    mirepcb.cpp
    modedit.cpp
    modedit_onclick.cpp
    modeditoptions.cpp
    modedit_undo_redo.cpp
    moduleframe.cpp
    modules.cpp
#   move_copy_track.cpp <-- not used
    move-drag_pads.cpp
    move_or_drag_track.cpp
    muonde.cpp
    muwave_command.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    pcbcfg.cpp
    pcbframe.cpp
    pcbnew.cpp
    pcbplot.cpp
    plotgerb.cpp
    plothpgl.cpp
    plotps.cpp
    plotdxf.cpp
    plot_rtn.cpp
    queue.cpp
    ratsnest.cpp
    router.cpp
    set_color.cpp
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
    via_edit.cpp
    work.cpp
    xchgmod.cpp
    zones_by_polygon.cpp
    zones_by_polygon_fill_functions.cpp
    zones_convert_brd_items_to_polygons.cpp
    zone_filling_algorithm.cpp
    zones_functions_for_undo_redo.cpp
    zones_polygons_insulated_copper_islands.cpp
    zones_polygons_test_connections.cpp
    zones_test_and_combine_areas.cpp
    )

set(PCBNEW_EXTRA_SRCS
    ../share/setpage.cpp
)

if(WIN32)
    if(MINGW)
        # PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler(pcbnew)
    else(MINGW)
        set(PCBNEW_RESOURCES pcbnew.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(PCBNEW_RESOURCES pcbnew.icns pcbnew_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcbnew_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE pcbnew.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcbnew)
    set(MACOSX_BUNDLE_NAME pcbnew)
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

add_executable(pcbnew WIN32 MACOSX_BUNDLE ${PCBNEW_SRCS} ${PCBNEW_EXTRA_SRCS} ${PCBNEW_RESOURCES})

if(APPLE)
    set_target_properties(pcbnew PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(pcbnew 3d-viewer common pcbcommon polygon bitmaps kbool ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES})

install(TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)


# This one gets made only when testing.
add_executable(dsntest EXCLUDE_FROM_ALL dsn.cpp)
target_link_libraries(dsntest common ${wxWidgets_LIBRARIES})

# This one gets made only when testing.
add_executable(specctra_test EXCLUDE_FROM_ALL specctra.cpp dsn.cpp)
target_link_libraries(specctra_test common ${wxWidgets_LIBRARIES})
