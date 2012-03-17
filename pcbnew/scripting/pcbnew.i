%module pcbnew
%include "kicad.i"

// this is what it must be included in the wrapper .cxx code to compile

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
	#include <class_zone_settings.h>
	#include <class_netclass.h>
	#include <class_netinfo.h>
	#include <layers_id_colors_and_visibility.h>
	#include <pcbnew_scripting_helpers.h>
      
	BOARD *GetBoard(); /* get current editor board */
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
%include <class_zone_settings.h>
%include <class_netclass.h>
%include <class_netinfo.h>
%include <layers_id_colors_and_visibility.h>

/* the IO_ERROR exception handler, not working yet... */
%exception
{
	try {
	$function
	}
	catch (IO_ERROR e) {
	 	PyErr_SetString(PyExc_IOError,"IO error");
		return NULL;
	}
}

%include <pcbnew_scripting_helpers.h>

#ifdef BUILD_WITH_PLUGIN
  %include <io_mgr.h>
  %include <kicad_plugin.h>
#endif

/* this is to help python with the * accessor of DLIST templates */

%rename(Get) operator BOARD_ITEM*; 
%rename(Get) operator TRACK*; 
%rename(Get) operator D_PAD*; 
%rename(Get) operator MODULE*; 


BOARD *GetBoard();

// we must translate C++ templates to scripting languages

%template(BOARD_ITEM_List) DLIST<BOARD_ITEM>;
%template(MODULE_List)     DLIST<MODULE>;
%template(TRACK_List)      DLIST<TRACK>;
%template(PAD_List)        DLIST<D_PAD>;

/* TODO: -the std::* compilatio is broken with some swig + gcc combinations 
 *        see kicad.i for more information.
 * %template(MARKER_Vector) std::vector<MARKER_PCB*>;
 * %template(ZONE_CONTAINER_Vector) std::vector<ZONE_CONTAINER*>;
 */

