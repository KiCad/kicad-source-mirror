
include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ../potrace
    ../polygon/kbool/include
    ../common
    ${INC_AFTER}
    )

set(BITMAP2COMPONENT_SRCS
    bitmap2component.cpp
    bitmap2cmp_gui_base
    bitmap2cmp_gui
    )

if(WIN32)
    if(MINGW)
        # BITMAP2COMPONENT_RESOURCES variable is set by the macro.
        mingw_resource_compiler(bitmap2component)
    else(MINGW)
        set(BITMAP2COMPONENT_RESOURCES bitmap2component.rc)
    endif(MINGW)
endif(WIN32)


if(APPLE)
    set(BITMAP2COMPONENT_RESOURCES bitmap2component.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/bitmap2component.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE bitmap2component.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.bitmap2component)
endif(APPLE)



add_executable(bitmap2component WIN32 MACOSX_BUNDLE
                            ${BITMAP2COMPONENT_SRCS}
                            ${BITMAP2COMPONENT_RESOURCES})


if(APPLE)
    set_target_properties(bitmap2component PROPERTIES MACOSX_BUNDLE_INFO_PLIST
        ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries( bitmap2component common polygon bitmaps
                        ${wxWidgets_LIBRARIES}
                        potrace
                        )

install(TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

