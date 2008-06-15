add_definitions(-DPCBNEW)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ../3d-viewer
    ../polygon)

# Many of the commented out ones are nested in *.cpp files for dialogs
set(PCBNEW_SRCS
    affiche.cpp
    attribut.cpp
    automove.cpp
    autoplac.cpp
    autorout.cpp
    basepcbframe.cpp
    block.cpp
    block_module_editor.cpp
    board.cpp
    class_board.cpp
    class_board_item.cpp
    class_cotation.cpp
    class_drawsegment.cpp
    class_drc_item.cpp
    class_edge_mod.cpp
    class_equipot.cpp
    class_marker.cpp
    class_mire.cpp
    class_module.cpp
    class_pad.cpp
    classpcb.cpp
    class_pcb_text.cpp
    class_text_mod.cpp
    class_track.cpp
    class_zone.cpp
    clean.cpp
#   cleaningoptions_dialog.cpp
    collectors.cpp
    connect.cpp
    controle.cpp
#   copy_track.cpp <-- not used
    cotation.cpp
    cross-probing.cpp
    deltrack.cpp
#   dialog_display_options.cpp
#   dialog_drc.cpp
    dialog_edit_mod_text.cpp
#   dialog_edit_module.cpp
    dialog_freeroute_exchange.cpp
#   dialog_gendrill.cpp
    dialog_general_options.cpp
#   dialog_graphic_items_options.cpp
#   dialog_initpcb.cpp
#   dialog_netlist.cpp
#   dialog_pad_edit.cpp
    dialog_setup_libs.cpp
#   dialog_track_options.cpp
#   dialog_zones_by_polygon.cpp
    dist.cpp
    dragsegm.cpp
    drc.cpp
    dsn.cpp
#   dupltrac.cpp <-- not used
    edgemod.cpp
    edit.cpp
    editedge.cpp
    editmod.cpp
    editpads.cpp
    editrack.cpp
    editrack-part2.cpp
    edit_track_width.cpp
    edtxtmod.cpp
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
    lay2plot.cpp
    librairi.cpp
    loadcmp.cpp
    locate.cpp
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
    pcbtexte.cpp
    plotgerb.cpp
    plothpgl.cpp
    plotps.cpp
    plot_rtn.cpp
    queue.cpp
    ratsnest.cpp
    router.cpp
    sel_layer.cpp
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
    tracemod.cpp
    tracepcb.cpp
    track.cpp
    tr_modif.cpp
    trpiste.cpp
    undelete.cpp
    via_edit.cpp
    work.cpp
    xchgmod.cpp
    zone_filling_algorithm.cpp
    zones_by_polygon.cpp
    zones_test_and_combine_areas.cpp)

set(PCBNEW_EXTRA_SRCS
    ../share/drawframe.cpp
    ../share/drawpanel.cpp
    ../share/setpage.cpp
    ../share/wxprint.cpp
    ../share/zoom.cpp)

if(WIN32)
    if(MINGW)
        # PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler(pcbnew)
    else(MINGW)
        set(PCBNEW_RESOURCES pcbnew.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(PCBNEW_RESOURCES pcbnew.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set(MACOSX_BUNDLE_ICON_FILE pcbnew.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcbnew)
    set(MACOSX_BUNDLE_NAME pcbnew)
endif(APPLE)

add_executable(pcbnew WIN32 MACOSX_BUNDLE ${PCBNEW_SRCS} ${PCBNEW_EXTRA_SRCS} ${PCBNEW_RESOURCES})

target_link_libraries(pcbnew 3d-viewer common polygon kbool bitmaps ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})

install(TARGETS pcbnew
        RUNTIME DESTINATION ${KICAD_BIN}
        BUNDLE DESTINATION ${KICAD_BIN}
        COMPONENT binary)


# This one gets made only when testing.
add_executable(dsntest EXCLUDE_FROM_ALL dsn.cpp)
target_link_libraries(dsntest common ${wxWidgets_LIBRARIES})

# This one gets made only when testing.
add_executable(specctra_test EXCLUDE_FROM_ALL specctra.cpp dsn.cpp)
target_link_libraries(specctra_test common ${wxWidgets_LIBRARIES})
