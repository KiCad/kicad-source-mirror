%module pcbnew
%import "kicad.i"


%{
	#include <class_board_item.h>
	#include <class_board.h>
	#include <class_module.h>
	#include <class_track.h>	
	#include <class_pad.h>
%}

%include <class_board_item.h>
%include <class_board.h>
%include <class_module.h>
%include <class_track.h>




/*%template(BOARD_ITEM_List) DLIST<BOARD_ITEM>;
%template(MODULE_List) DLIST<MODULE>;
%template(TRACK_List) DLIST<TRACK>;
%template(PAD_List) DLIST<D_PAD>;
*/
