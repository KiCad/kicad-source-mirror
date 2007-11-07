# We define the include paths here, our minimal source dir is one, 
# and also the include dirs defined by wxWidgets
#INCLUDE_DIRECTORIES()
 
# For convenience we define the sources as a variable. You can add 
# header files and cpp / c files and CMake will sort them out
SET(SRCS    	confirm
		base_struct
		copy_to_clipboard
		basicframe
		common_plot_functions
		common_plotPS_functions
		common_plotHPGL_functions
		hotkeys_basic
		drawtxt
		wxwineda
		string
		gr_basic
		gestfich
		trigo
		selcolor
		bitmaps
		common
		eda_doc
		toolbars
		displlst
		edaappl
		block_commande
		msgpanel
		projet_config
		get_component_dialog
		eda_dde
		worksheet
		base_screen
		dcsvg
   )
 
# If we build for windows systems, we also include the resource file
# containing the manifest, icon and other resources
#IF(WIN32)
#  SET(SRCS ${SRCS} kicad.rc)
#ENDIF(WIN32)
 
# Here we define the executable kicad.exe or kicad on other systems
# the above paths and defines will be used in this build
ADD_LIBRARY(common ${SRCS})
    
# We add to our target 'kicad' the wxWidgets libraries. These are
# set for us by the find script. If you need other libraries, you
# can add them here as well. 
#TARGET_LINK_LIBRARIES(kicad ${wxWidgets_LIBRARIES} )
