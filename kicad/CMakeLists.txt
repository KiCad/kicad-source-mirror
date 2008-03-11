add_definitions(-DKICAD)

include_directories(bitmaps)

set(KICAD_SRCS
    buildmnu.cpp
    commandframe.cpp
    files-io.cpp
    kicad.cpp
    mainframe.cpp
    preferences.cpp
    prjconfig.cpp
    treeprj_datas.cpp
    treeprj_frame.cpp)

if(WIN32)
    if(MINGW)
        # resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o
            COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${wxWidgets_ROOT_DIR}/include
                        -i ${CMAKE_CURRENT_SOURCE_DIR}/kicad.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o)
        set(KICAD_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o)
    else(MINGW)
        set(KICAD_RESOURCES kicad.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(KICAD_RESOURCES kicad.r)
endif(APPLE)

add_executable(kicad WIN32 MACOSX_BUNDLE ${KICAD_SRCS} ${KICAD_EXTRA_SRCS} ${KICAD_RESOURCES})

target_link_libraries(kicad common ${wxWidgets_LIBRARIES})

install(TARGETS kicad RUNTIME DESTINATION ${KICAD_BIN} COMPONENT binary)

if(KICAD_MINIZIP)
    add_subdirectory(minizip)
endif(KICAD_MINIZIP)
