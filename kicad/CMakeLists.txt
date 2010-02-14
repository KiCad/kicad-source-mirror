add_definitions(-DKICAD)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                    )

set(KICAD_SRCS
    commandframe.cpp
    files-io.cpp
    kicad.cpp
    kicad-python.cpp
    mainframe.cpp
    menubar.cpp
    preferences.cpp
    prjconfig.cpp
    treeprj_datas.cpp
    treeprj_frame.cpp)

if(WIN32)
    if(MINGW)
        # KICAD_RESOURCES variable is set by the macro.
        mingw_resource_compiler(kicad)
    else(MINGW)
        set(KICAD_RESOURCES kicad.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(KICAD_RESOURCES kicad.icns kicad_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/kicad.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/kicad_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE kicad.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad)
    set(MACOSX_BUNDLE_NAME kicad)
endif(APPLE)

if(APPLE)
	add_executable(KiCad MACOSX_BUNDLE ${KICAD_SRCS} ${KICAD_EXTRA_SRCS} ${KICAD_RESOURCES})
else(APPLE)
	add_executable(kicad WIN32 ${KICAD_SRCS} ${KICAD_EXTRA_SRCS} ${KICAD_RESOURCES})
endif(APPLE)

if(APPLE)
    set_target_properties(KiCad PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
	target_link_libraries(KiCad common bitmaps ${wxWidgets_LIBRARIES})
install(TARGETS KiCad
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
else(APPLE)
	target_link_libraries(kicad common bitmaps polygon ${wxWidgets_LIBRARIES} ${GDI_PLUS_LIBRARIES})
	install(TARGETS kicad
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
endif(APPLE)

if(KICAD_MINIZIP)
    add_subdirectory(minizip)
endif(KICAD_MINIZIP)
