ADD_DEFINITIONS(-DGERBVIEW -DPCBNEW)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	../3d-viewer
	../cvpcb
	../pcbnew)

SET(GERBVIEW_SRCS
	affiche.cpp
	block.cpp
	controle.cpp
	dcode.cpp
	deltrack.cpp
	edit.cpp
	export_to_pcbnew.cpp
	files.cpp
	gerberframe.cpp
	gerbview_config.cpp
	gerbview.cpp
	hotkeys.cpp
	initpcb.cpp
	lay2plot.cpp
	locate.cpp
	onrightclick.cpp
	options.cpp
	pcbplot.cpp
#	pcbtexte.cpp
#	process_config.cpp
	readgerb.cpp
	reglage.cpp
	rs274d.cpp
	rs274x.cpp
	select_layers_to_pcb.cpp
	set_color.cpp
	tool_gerber.cpp
	tracepcb.cpp
	trpiste.cpp
	undelete.cpp)

SET(GERBVIEW_EXTRA_SRCS
	../pcbnew/basepcbframe.cpp
	../pcbnew/class_board.cpp
	../pcbnew/class_marker.cpp
	../pcbnew/class_pcb_text.cpp
	../pcbnew/class_track.cpp
	../pcbnew/classpcb.cpp
	../pcbnew/collectors.cpp
	../pcbnew/sel_layer.cpp

	../share/drawframe.cpp
	../share/drawpanel.cpp
	../share/infospgm.cpp
	../share/setpage.cpp
	../share/wxprint.cpp
	../share/zoom.cpp)

IF(WIN32)
	SET(GERBVIEW_RESOURCES gerbview.rc)
ENDIF(WIN32)
IF(APPLE)
	SET(GERBVIEW_RESOURCES gerbview.r)
ENDIF(APPLE)

ADD_EXECUTABLE(gerbview WIN32 MACOSX_BUNDLE ${GERBVIEW_SRCS} ${GERBVIEW_EXTRA_SRCS} ${GERBVIEW_RESOURCES})

TARGET_LINK_LIBRARIES(gerbview common 3d-viewer ${wxWidgets_LIBRARIES})

INSTALL(TARGETS gerbview RUNTIME DESTINATION ${KICAD_BIN})
