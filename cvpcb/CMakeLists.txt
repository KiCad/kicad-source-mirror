ADD_DEFINITIONS(-DCVPCB)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	bitmaps
	../3d-viewer
	../polygon
	../pcbnew)

SET(CVPCB_SRCS
	autosel.cpp
	cfg.cpp
	class_cvpcb.cpp
	cvframe.cpp
	cvpcb.cpp
#	dialog_cvpcb_config.cpp
	dialog_display_options.cpp
	displayframe.cpp
	genequiv.cpp
#	genorcad.cpp
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

SET(CVPCB_EXTRA_SRCS
	../pcbnew/basepcbframe.cpp
	../pcbnew/class_board.cpp
	../pcbnew/class_zone.cpp
	../pcbnew/class_cotation.cpp
	../pcbnew/class_edge_mod.cpp
	../pcbnew/class_equipot.cpp
	../pcbnew/class_mire.cpp
	../pcbnew/class_module.cpp
	../pcbnew/class_pad.cpp
	../pcbnew/class_pcb_text.cpp
	../pcbnew/class_text_mod.cpp
	../pcbnew/class_track.cpp
	../pcbnew/classpcb.cpp
	../pcbnew/collectors.cpp
	../pcbnew/ioascii.cpp
	../pcbnew/tracemod.cpp

	../share/drawframe.cpp
	../share/drawpanel.cpp
	../share/infospgm.cpp
	../share/zoom.cpp)

IF(WIN32)
	IF(MINGW)
		# resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o
			COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR}
					    -i ${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.rc
					    -o ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o)
		SET(CVPCB_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/cvpcb_rc.o)
	ELSE(MINGW)
		SET(CVPCB_RESOURCES cvpcb.rc)
	ENDIF(MINGW)
ENDIF(WIN32)
IF(APPLE)
	SET(CVPCB_RESOURCES cvpcb.r)
ENDIF(APPLE)

ADD_EXECUTABLE(cvpcb WIN32 MACOSX_BUNDLE ${CVPCB_SRCS} ${CVPCB_EXTRA_SRCS} ${CVPCB_RESOURCES})

TARGET_LINK_LIBRARIES(cvpcb common 3d-viewer ${wxWidgets_LIBRARIES})

INSTALL(TARGETS cvpcb RUNTIME DESTINATION ${KICAD_BIN})
