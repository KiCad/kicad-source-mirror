add_definitions(-DGERBVIEW -DPCBNEW)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../cvpcb
                    ../pcbnew
                    ../polygon)

set(GERBVIEW_SRCS
    affiche.cpp
    block.cpp
    controle.cpp
    dcode.cpp
    deltrack.cpp
    edit.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerberframe.cpp
    gerbview_config.cpp
    gerbview.cpp
    hotkeys.cpp
    initpcb.cpp
    lay2plot.cpp
    locate.cpp
    onrightclick.cpp
    options.cpp
    pcbplot.cpp
#   pcbtexte.cpp
#   process_config.cpp
    readgerb.cpp
    reglage.cpp
    rs274d.cpp
    rs274x.cpp
    select_layers_to_pcb.cpp
    set_color.cpp
#   struct.cpp <-- not used
    tool_gerber.cpp
    tracepcb.cpp
    trpiste.cpp
    undelete.cpp)

set(GERBVIEW_EXTRA_SRCS
    ../pcbnew/basepcbframe.cpp
    ../pcbnew/class_board.cpp
    ../pcbnew/class_board_item.cpp
    ../pcbnew/class_board_connected_item.cpp
    ../pcbnew/class_drawsegment.cpp
    ../pcbnew/class_drc_item.cpp
    ../pcbnew/class_marker.cpp
    ../pcbnew/class_pcb_text.cpp
    ../pcbnew/class_track.cpp
    ../pcbnew/class_zone.cpp
    ../pcbnew/class_zone_setting.cpp
    ../pcbnew/classpcb.cpp
    ../pcbnew/collectors.cpp
    ../pcbnew/sel_layer.cpp

    ../share/drawframe.cpp
    ../share/drawpanel.cpp
    ../share/setpage.cpp
    ../share/wxprint.cpp
    ../share/zoom.cpp)

if(WIN32)
    if(MINGW)
        # GERBVIEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler(gerbview)
    else(MINGW)
        set(GERBVIEW_RESOURCES gerbview.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(GERBVIEW_RESOURCES gerbview.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/gerbview.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE gerbview.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.gerbview)
    set(MACOSX_BUNDLE_NAME gerbview)
endif(APPLE)

add_executable(gerbview WIN32 MACOSX_BUNDLE ${GERBVIEW_SRCS} ${GERBVIEW_EXTRA_SRCS} ${GERBVIEW_RESOURCES})

target_link_libraries(gerbview 3d-viewer common polygon bitmaps kbool ${wxWidgets_LIBRARIES})

install(TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
