%module pcbnew

%include "kicad.i"

%{
        #include <wx_python_helpers.h>
	#include <class_board_item.h>
	#include <class_board.h>
	#include <class_module.h>
	#include <class_track.h>	
	#include <class_pad.h>
	#include <class_netinfo.h>
        #include <class_pcb_text.h>
	#include <class_dimension.h>
	#include <dlist.h>
      



	BOARD *GetBoard();
%}

#ifdef BUILD_WITH_PLUGIN
%{
	#include <io_mgr.h>
	#include <kicad_plugin.h>
%}
#endif


%include <class_board_item.h>
%include <class_board.h>
%include <class_module.h>
%include <class_track.h>
%include <class_pad.h>
%include <class_netinfo.h>
%include <class_pcb_text.h>
%include <class_dimension.h>
%include <dlist.h>

#ifdef BUILD_WITH_PLUGIN
%include <io_mgr.h>
%include <kicad_plugin.h>
#endif

%rename(Get) operator BOARD_ITEM*; 
%rename(Get) operator TRACK*; 
%rename(Get) operator D_PAD*; 
%rename(Get) operator MODULE*; 


BOARD *GetBoard();

// se must translate C++ templates to scripting languages

%template(BOARD_ITEM_List) DLIST<BOARD_ITEM>;
%template(MODULE_List) DLIST<MODULE>;
%template(TRACK_List) DLIST<TRACK>;
%template(PAD_List) DLIST<D_PAD>;


