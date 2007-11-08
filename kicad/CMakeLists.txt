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
	SET(KICAD_RESOURCES kicad.rc)
ENDIF(WIN32)
IF(APPLE)
	SET(KICAD_RESOURCES kicad.r)
ENDIF(APPLE)

ADD_EXECUTABLE(kicad WIN32 MACOSX_BUNDLE ${KICAD_SRCS} ${KICAD_EXTRA_SRCS} ${KICAD_RESOURCES})

TARGET_LINK_LIBRARIES(kicad common ${wxWidgets_LIBRARIES})

INSTALL(PROGRAMS kicad DESTINATION /bin)

#ADD_SUBDIRECTORY(minizip)
