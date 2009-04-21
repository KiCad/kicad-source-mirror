add_definitions(-DCVPCB)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../pcbnew
                    ../polygon)

set(CVPCB_SRCS
    autosel.cpp
    cfg.cpp
    class_cvpcb.cpp
    cvframe.cpp
    cvpcb.cpp
    dialog_cvpcb_config.cpp
    dialog_cvpcb_config_fbp.cpp
    dialog_display_options.cpp
    displayframe.cpp
    genequiv.cpp
    init.cpp
    listboxes.cpp
    listlib.cpp
    loadcmp.cpp
    memoire.cpp
    menucfg.cpp
    readschematicnetlist.cpp
    savecmp.cpp
    setvisu.cpp
    tool_cvpcb.cpp
    writenetlistpcbnew.cpp)

set(CVPCB_EXTRA_SRCS
#    ../pcbnew/class_board_item.cpp
#    ../pcbnew/class_drawsegment.cpp
#    ../pcbnew/class_edge_mod.cpp
#    ../pcbnew/class_equipot.cpp
#    ../pcbnew/class_module.cpp
#    ../pcbnew/class_text_mod.cpp
    ../pcbnew/ioascii.cpp
#    ../pcbnew/tracemod.cpp
)

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

target_link_libraries(cvpcb 3d-viewer common pcbcommon polygon bitmaps kbool ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES})

install(TARGETS cvpcb
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
