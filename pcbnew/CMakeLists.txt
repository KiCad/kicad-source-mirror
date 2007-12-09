ADD_DEFINITIONS(-DPCBNEW)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	bitmaps
	../3d-viewer)

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
	dist.cpp
#	docedit.cpp
	dragsegm.cpp
	drc.cpp
	edgemod.cpp
	edit.cpp
	editedge.cpp
	editmod.cpp
	editpads.cpp
	editrack.cpp
	editrack-part2.cpp
#	editrout.cpp
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
#	move_copy_track.cpp
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
#	reglage.cpp
	router.cpp
	sel_layer.cpp
	set_color.cpp
	set_grid.cpp
	solve.cpp
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
	zones.cpp)

SET(PCBNEW_EXTRA_SRCS
	../share/drawframe.cpp
	../share/drawpanel.cpp
	../share/infospgm.cpp
	../share/setpage.cpp
	../share/wxprint.cpp
	../share/zoom.cpp)

IF(WIN32)
	SET(PCBNEW_RESOURCES pcbnew.rc)
ENDIF(WIN32)
IF(APPLE)
	SET(PCBNEW_RESOURCES pcbnew.r)
ENDIF(APPLE)

ADD_EXECUTABLE(pcbnew WIN32 MACOSX_BUNDLE ${PCBNEW_SRCS} ${PCBNEW_EXTRA_SRCS} ${PCBNEW_RESOURCES})

TARGET_LINK_LIBRARIES(pcbnew common 3d-viewer ${wxWidgets_LIBRARIES})

INSTALL(TARGETS pcbnew RUNTIME DESTINATION ${KICAD_BIN})
