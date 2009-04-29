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
    menucfg.cpp
    readschematicnetlist.cpp
    savecmp.cpp
    setvisu.cpp
    tool_cvpcb.cpp
    writenetlistpcbnew.cpp)

if(WIN32)
    if(MINGW)
        # CVPCB_RESOURCES variable is set by the macro.
        mingw_resource_compiler(cvpcb)
    else(MINGW)
        set(CVPCB_RESOURCES cvpcb.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(CVPCB_RESOURCES cvpcb.icns cvpcb_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cvpcb_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE cvpcb.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.cvpcb)
    set(MACOSX_BUNDLE_NAME cvpcb)
endif(APPLE)

add_executable(cvpcb WIN32 MACOSX_BUNDLE ${CVPCB_SRCS} ${CVPCB_RESOURCES})

if(APPLE)
    set_target_properties(cvpcb PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(cvpcb 3d-viewer common pcbcommon polygon bitmaps kbool ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES})

install(TARGETS cvpcb
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
