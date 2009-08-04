add_definitions(-DGERBVIEW -DPCBNEW)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../cvpcb
                    ../pcbnew
                    ../polygon)

set(GERBVIEW_SRCS
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
    readgerb.cpp
    reglage.cpp
    rs274d.cpp
    rs274x.cpp
    select_layers_to_pcb.cpp
    set_color.cpp
    tool_gerber.cpp
    tracepcb.cpp
    trpiste.cpp )

set(GERBVIEW_EXTRA_SRCS
#    ../pcbnew/class_board_item.cpp
#    ../pcbnew/class_drawsegment.cpp
#    ../pcbnew/undelete.cpp

    ../share/setpage.cpp
    ../pcbnew/dialog_print_using_printer.cpp
)

if(WIN32)
    if(MINGW)
        # GERBVIEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler(gerbview)
    else(MINGW)
        set(GERBVIEW_RESOURCES gerbview.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(GERBVIEW_RESOURCES gerbview.icns gerbview_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/gerbview.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/gerbview_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE gerbview.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.gerbview)
    set(MACOSX_BUNDLE_NAME gerbview)
endif(APPLE)

add_executable(gerbview WIN32 MACOSX_BUNDLE ${GERBVIEW_SRCS} ${GERBVIEW_EXTRA_SRCS} ${GERBVIEW_RESOURCES})

if(APPLE)
    set_target_properties(gerbview PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(gerbview common pcbcommon 3d-viewer polygon bitmaps kbool ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES})

install(TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
