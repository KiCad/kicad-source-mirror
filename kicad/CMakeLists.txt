ADD_DEFINITIONS(-DKICAD)

INCLUDE_DIRECTORIES(bitmaps)

SET(KICAD_SRCS
	buildmnu.cpp
	commandframe.cpp
	files-io.cpp
	kicad.cpp
	mainframe.cpp
	preferences.cpp
	prjconfig.cpp
	treeprj_datas.cpp
	treeprj_frame.cpp)

SET(KICAD_EXTRA_SRCS
	../share/infospgm.cpp)

IF(WIN32)
	IF(MINGW)
		# resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o
			COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR}
					    -i ${CMAKE_CURRENT_SOURCE_DIR}/kicad.rc
					    -o ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o)
		SET(KICAD_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/kicad_rc.o)
	ELSE(MINGW)
		SET(KICAD_RESOURCES kicad.rc)
	ENDIF(MINGW)
ENDIF(WIN32)
IF(APPLE)
	SET(KICAD_RESOURCES kicad.r)
ENDIF(APPLE)

ADD_EXECUTABLE(kicad WIN32 MACOSX_BUNDLE ${KICAD_SRCS} ${KICAD_EXTRA_SRCS} ${KICAD_RESOURCES})

TARGET_LINK_LIBRARIES(kicad common ${wxWidgets_LIBRARIES})

INSTALL(TARGETS kicad RUNTIME DESTINATION ${KICAD_BIN})

ADD_SUBDIRECTORY(minizip)
