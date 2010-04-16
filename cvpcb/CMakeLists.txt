add_definitions(-DCVPCB)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../pcbnew
                    ../polygon
                    ${CMAKE_SOURCE_DIR}/common)
##
# Name of target on OSX is also the title of the application
# on other targets this should be as normal
##
if(APPLE)
	set(CVPCB_NAME CvPCB)
else(APPLE)
	set(CVPCB_NAME cvpcb)
endif(APPLE)

set(CVPCB_SRCS
    autosel.cpp
    cfg.cpp
    class_cvpcb.cpp
	class_components_listbox.cpp
    class_DisplayFootprintsFrame.cpp
	class_footprints_listbox.cpp
    cvframe.cpp
    cvpcb.cpp
    dialog_cvpcb_config.cpp
    dialog_cvpcb_config_fbp.cpp
    dialog_display_options.cpp
    dialog_display_options_base.cpp
    dummy_functions.cpp
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

###
# CvPCB.app resources
###
if(APPLE)
    set(CVPCB_RESOURCES cvpcb.icns cvpcb_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/cvpcb_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE cvpcb.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.cvpcb)
endif(APPLE)

add_executable(${CVPCB_NAME} WIN32 MACOSX_BUNDLE ${CVPCB_SRCS} ${CVPCB_RESOURCES})

###
# CvPCB.app creation
###
if(APPLE)
    set_target_properties(${CVPCB_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(${CVPCB_NAME} 3d-viewer common pcbcommon polygon bitmaps kbool
                      ${OPENGL_LIBRARIES} ${wxWidgets_LIBRARIES} ${GDI_PLUS_LIBRARIES})

install(TARGETS ${CVPCB_NAME}
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
