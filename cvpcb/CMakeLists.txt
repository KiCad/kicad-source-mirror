add_definitions(-DCVPCB)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../3d-viewer
    ../pcbnew
    ../polygon)

set(CVPCB_SRCS
    autosel.cpp
    cfg.cpp
    class_cvpcb.cpp
    cvframe.cpp
    cvpcb.cpp
#   dialog_cvpcb_config.cpp
    dialog_display_options.cpp
    displayframe.cpp
    genequiv.cpp
#   genorcad.cpp
    init.cpp
    listboxes.cpp
    listlib.cpp
    loadcmp.cpp
    memoire.cpp
    menucfg.cpp
    rdpcad.cpp
    readschematicnetlist.cpp
    savecmp.cpp
    setvisu.cpp
    tool_cvpcb.cpp
    viewlnet.cpp
    viewlogi.cpp
    writenetlistpcbnew.cpp)

set(CVPCB_EXTRA_SRCS
    ../pcbnew/basepcbframe.cpp
    ../pcbnew/class_board.cpp
    ../pcbnew/class_board_item.cpp
    ../pcbnew/class_cotation.cpp
    ../pcbnew/class_drawsegment.cpp
    ../pcbnew/class_edge_mod.cpp
    ../pcbnew/class_equipot.cpp
    ../pcbnew/class_mire.cpp
    ../pcbnew/class_module.cpp
    ../pcbnew/class_pad.cpp
    ../pcbnew/class_pcb_text.cpp
    ../pcbnew/class_text_mod.cpp
    ../pcbnew/class_track.cpp
    ../pcbnew/class_zone.cpp
    ../pcbnew/classpcb.cpp
    ../pcbnew/collectors.cpp
    ../pcbnew/ioascii.cpp
    ../pcbnew/tracemod.cpp

    ../share/drawframe.cpp
    ../share/drawpanel.cpp
    ../share/zoom.cpp)

if(WIN32)
    if(MINGW)
        # CVPCB_RESOURCES variable is set by the macro.
        mingw_resource_compiler(cvpcb)
    else(MINGW)
        set(CVPCB_RESOURCES cvpcb.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(CVPCB_RESOURCES cvpcb.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set(MACOSX_BUNDLE_ICON_FILE cvpcb.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.cvpcb)
    set(MACOSX_BUNDLE_NAME cvpcb)
endif(APPLE)

add_executable(cvpcb WIN32 MACOSX_BUNDLE ${CVPCB_SRCS} ${CVPCB_EXTRA_SRCS} ${CVPCB_RESOURCES})

target_link_libraries(cvpcb 3d-viewer common polygon kbool bitmaps ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})

if(NOT APPLE)
    install(TARGETS cvpcb
        RUNTIME DESTINATION ${KICAD_BIN}
        COMPONENT binary)
else(NOT APPLE)
    install(TARGETS cvpcb
        RUNTIME DESTINATION ${KICAD_BIN}
        BUNDLE DESTINATION ${KICAD_BIN}
        COMPONENT binary)
endif(NOT APPLE)
