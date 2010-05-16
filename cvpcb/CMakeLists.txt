add_definitions(-DCVPCB)

###
# Includes
###
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    ../3d-viewer
                    ../pcbnew
                    ../polygon
                    ${CMAKE_SOURCE_DIR}/common)

###
# Sources
###
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

###
# Windows resource file
###
if(WIN32)
    if(MINGW)
        # CVPCB_RESOURCES variable is set by the macro.
        mingw_resource_compiler(cvpcb)
    else(MINGW)
        set(CVPCB_RESOURCES cvpcb.rc)
    endif(MINGW)
endif(WIN32)

###
# Apple resource files
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

###
# Create the cvpcb executable
###
add_executable(cvpcb WIN32 MACOSX_BUNDLE
               ${CVPCB_SRCS}
               ${CVPCB_RESOURCES})

###
# Set properties for APPLE on cvpcb target
###
if(APPLE)
    set_target_properties(cvpcb PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

###
# Link executable target cvpcb with correct libraries
###
target_link_libraries(cvpcb 3d-viewer common pcbcommon polygon bitmaps kbool
                      ${OPENGL_LIBRARIES}
                      ${wxWidgets_LIBRARIES}
                      ${GDI_PLUS_LIBRARIES}
                     )
###
# Add cvpcb as install target
###
install(TARGETS cvpcb
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
