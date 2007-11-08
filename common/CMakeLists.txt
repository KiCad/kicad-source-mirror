INCLUDE_DIRECTORIES(bitmaps)

SET(COMMON_SRCS
	base_screen.cpp
	base_struct.cpp
	basicframe.cpp
	bitmaps.cpp
	block_commande.cpp
	common.cpp
	common_plot_functions.cpp
	common_plotHPGL_functions.cpp
	common_plotPS_functions.cpp
	confirm.cpp
	copy_to_clipboard.cpp
	dcsvg.cpp
	displlst.cpp
	drawtxt.cpp
	edaappl.cpp
	eda_dde.cpp
	eda_doc.cpp
#	edamenu.cpp
	gestfich.cpp
	get_component_dialog.cpp
	gr_basic.cpp
	hotkeys_basic.cpp
	msgpanel.cpp
	projet_config.cpp
#	pyhandler.cpp
	selcolor.cpp
	string.cpp
	toolbars.cpp
	trigo.cpp
	worksheet.cpp
	wxwineda.cpp)


ADD_LIBRARY(common ${COMMON_SRCS})
