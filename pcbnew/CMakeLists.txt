ADD_DEFINITIONS(-DPCBNEW)

FIND_PACKAGE(Boost)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${Boost_INCLUDE_DIRS}
	bitmaps
	../3d-viewer
	../polygon)

# Many of the commented out ones are nested in *.cpp files for dialogs
SET(PCBNEW_SRCS
	affiche.cpp
	attribut.cpp
	automove.cpp
	autoplac.cpp
	autorout.cpp
	basepcbframe.cpp
	block.cpp
	block_module_editor.cpp
	board.cpp
	class_board.cpp
	class_board_item.cpp
	class_cotation.cpp
	class_drawsegment.cpp
	class_drc_item.cpp
	class_edge_mod.cpp
	class_equipot.cpp
	class_marker.cpp
	class_mire.cpp
	class_module.cpp
	class_pad.cpp
	classpcb.cpp
	class_pcb_text.cpp
	class_text_mod.cpp
	class_track.cpp
	class_zone.cpp
	clean.cpp
#	cleaningoptions_dialog.cpp
	collectors.cpp
	connect.cpp
	controle.cpp
	copy_track.cpp
	cotation.cpp
	cross-probing.cpp
	deltrack.cpp
#	dialog_display_options.cpp
#	dialog_drc.cpp
	dialog_edit_mod_text.cpp
#	dialog_edit_module.cpp
	dialog_general_options.cpp
#	dialog_graphic_items_options.cpp
#	dialog_initpcb.cpp
#	dialog_netlist.cpp
#	dialog_pad_edit.cpp
	dialog_setup_libs.cpp
#	dialog_track_options.cpp
#	dialog_zones_by_polygon.cpp
	dist.cpp
#	docedit.cpp <-- not used
	dragsegm.cpp
	drc.cpp
#	dsn.cpp
	edgemod.cpp
	edit.cpp
	editedge.cpp
	editmod.cpp
	editpads.cpp
	editrack.cpp
	editrack-part2.cpp
#	editrout.cpp <-- not used
	edit_track_width.cpp
	edtxtmod.cpp
	export_gencad.cpp
	files.cpp
	find.cpp
	gendrill.cpp
	gen_modules_placefile.cpp
	globaleditpad.cpp
	graphpcb.cpp
	hotkeys.cpp
	initpcb.cpp
	ioascii.cpp
	lay2plot.cpp
	librairi.cpp
	loadcmp.cpp
	locate.cpp
	menubarmodedit.cpp
	menubarpcb.cpp
	mirepcb.cpp
	modedit.cpp
	modedit_onclick.cpp
	modeditoptions.cpp
	modedit_undo_redo.cpp
	moduleframe.cpp
	modules.cpp
#	move_copy_track.cpp <-- not used
	move-drag_pads.cpp
	move_or_drag_track.cpp
	muonde.cpp
	muwave_command.cpp
	netlist.cpp
	onleftclick.cpp
	onrightclick.cpp
	pcbcfg.cpp
	pcbframe.cpp
	pcbnew.cpp
	pcbplot.cpp
	pcbtexte.cpp
	plotgerb.cpp
	plothpgl.cpp
	plotps.cpp
	plot_rtn.cpp
	queue.cpp
	ratsnest.cpp
	router.cpp
	sel_layer.cpp
	set_color.cpp
	set_grid.cpp
	solve.cpp
#	specctra.cpp
	surbrill.cpp
	swap_layers.cpp
	tool_modedit.cpp
	tool_onrightclick.cpp
	tool_pcb.cpp
	tracemod.cpp
	tracepcb.cpp
	track.cpp
	tr_modif.cpp
	trpiste.cpp
	undelete.cpp
	via_edit.cpp
	work.cpp
	xchgmod.cpp
	zone_filling_algorithm.cpp
	zones_by_polygon.cpp
#	zones.cpp
	zones_test_and_combine_areas.cpp)

SET(PCBNEW_EXTRA_SRCS
	../share/drawframe.cpp
	../share/drawpanel.cpp
	../share/infospgm.cpp
	../share/setpage.cpp
	../share/wxprint.cpp
	../share/zoom.cpp)

IF(WIN32)
	IF(MINGW)
		# resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_rc.o
			COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR}
					    -i ${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.rc
					    -o ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_rc.o)
		SET(PCBNEW_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_rc.o)
	ELSE(MINGW)
		SET(PCBNEW_RESOURCES pcbnew.rc)
	ENDIF(MINGW)
ENDIF(WIN32)
IF(APPLE)
	SET(PCBNEW_RESOURCES pcbnew.r)
ENDIF(APPLE)

ADD_EXECUTABLE(pcbnew WIN32 MACOSX_BUNDLE ${PCBNEW_SRCS} ${PCBNEW_EXTRA_SRCS} ${PCBNEW_RESOURCES})

# This one gets made only when testing.
#SET_SOURCE_FILES_PROPERTIES(dsn.cpp PROPERTIES COMPILE_FLAGS "-DSTANDALONE -Wall")
ADD_EXECUTABLE(dsntest EXCLUDE_FROM_ALL dsn.cpp)
TARGET_LINK_LIBRARIES(dsntest common ${wxWidgets_LIBRARIES})

# This one gets made only when testing.
#SET_SOURCE_FILES_PROPERTIES(specctra.cpp PROPERTIES COMPILE_FLAGS "-DSTANDALONE -Wall")
ADD_EXECUTABLE(specctra_test EXCLUDE_FROM_ALL specctra.cpp dsn.cpp )
TARGET_LINK_LIBRARIES(specctra_test common ${wxWidgets_LIBRARIES})


TARGET_LINK_LIBRARIES(pcbnew common 3d-viewer polygon ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})

INSTALL(TARGETS pcbnew RUNTIME DESTINATION ${KICAD_BIN} COMPONENT binary)
