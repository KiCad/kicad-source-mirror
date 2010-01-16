add_definitions(-DGERBVIEW -DPCBNEW)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../cvpcb
                    ../pcbnew
                    ../polygon)
##
# Name of target on OSX is also the title of the application
# on other targets this should be as normal
##
if(APPLE)
	set(GERBVIEW_NAME GerbView)
else(APPLE)
	set(GERBVIEW_NAME gerbview)
endif(APPLE)

set(GERBVIEW_SRCS
    block.cpp
    controle.cpp
    dcode.cpp
    deltrack.cpp
    dialog_print_using_printer.cpp
	dialog_print_using_printer_base.cpp
    dummy_functions.cpp
    edit.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerberframe.cpp
    gerbview_config.cpp
    gerbview_dialog_display_options_frame_base.cpp
    gerbview_dialog_display_options_frame.cpp
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
    tracepcb.cpp )

set(GERBVIEW_EXTRA_SRCS
    ../share/setpage.cpp
    ../pcbnew/printout_controler.cpp
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
endif(APPLE)

add_executable(${GERBVIEW_NAME} WIN32 MACOSX_BUNDLE ${GERBVIEW_SRCS} ${GERBVIEW_EXTRA_SRCS} ${GERBVIEW_RESOURCES})

if(APPLE)
    set_target_properties(${GERBVIEW_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(${GERBVIEW_NAME} common pcbcommon 3d-viewer polygon bitmaps kbool ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES})

install(TARGETS ${GERBVIEW_NAME}
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
