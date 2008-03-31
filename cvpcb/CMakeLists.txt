add_definitions(-DCVPCB)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    bitmaps
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
        # resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o
            COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${wxWidgets_ROOT_DIR}/include
                        -i ${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o)
        set(CVPCB_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o)
    else(MINGW)
        set(CVPCB_RESOURCES cvpcb.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(CVPCB_RESOURCES cvpcb.r)
endif(APPLE)

add_executable(cvpcb WIN32 MACOSX_BUNDLE ${CVPCB_SRCS} ${CVPCB_EXTRA_SRCS} ${CVPCB_RESOURCES})

target_link_libraries(cvpcb 3d-viewer common polygon ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})

install(TARGETS cvpcb RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary)
