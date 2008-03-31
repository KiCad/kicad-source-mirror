add_definitions(-DGERBVIEW -DPCBNEW)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
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
    tool_gerber.cpp
    tracepcb.cpp
    trpiste.cpp
    undelete.cpp)

set(GERBVIEW_EXTRA_SRCS
    ../pcbnew/basepcbframe.cpp
    ../pcbnew/class_board.cpp
    ../pcbnew/class_drawsegment.cpp
    ../pcbnew/class_drc_item.cpp
    ../pcbnew/class_marker.cpp
    ../pcbnew/class_pcb_text.cpp
    ../pcbnew/class_track.cpp
    ../pcbnew/class_zone.cpp
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
        # resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gerbview_rc.o
            COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${wxWidgets_ROOT_DIR}/include
                        -i ${CMAKE_CURRENT_SOURCE_DIR}/gerbview.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/gerbview_rc.o)
        set(GERBVIEW_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/gerbview_rc.o)
    else(MINGW)
        set(GERBVIEW_RESOURCES gerbview.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(GERBVIEW_RESOURCES gerbview.r)
endif(APPLE)

add_executable(gerbview WIN32 MACOSX_BUNDLE ${GERBVIEW_SRCS} ${GERBVIEW_EXTRA_SRCS} ${GERBVIEW_RESOURCES})

target_link_libraries(gerbview 3d-viewer common polygon ${wxWidgets_LIBRARIES})

install(TARGETS gerbview RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary)
