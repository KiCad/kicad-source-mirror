%module pcbnew
%import "kicad.i"

%{
	#include <class_board_item.h>
	#include <class_board.h>
	#include <class_module.h>
	#include <class_track.h>	
	#include <class_pad.h>
	#include <dlist.h>
        #include <wx_helpers.h>


	BOARD *GetBoard();
%}

%include <class_board_item.h>
%include <class_board.h>
%include <class_module.h>
%include <class_track.h>
%include <class_pad.h>
%include <dlist.h>

%rename(item) operator BOARD_ITEM*; 
%rename(item) operator TRACK*; 
%rename(item) operator D_PAD*; 
%rename(item) operator MODULE*; 


BOARD *GetBoard();



%template(BOARD_ITEM_List) DLIST<BOARD_ITEM>;
%template(MODULE_List) DLIST<MODULE>;
%template(TRACK_List) DLIST<TRACK>;
%template(PAD_List) DLIST<D_PAD>;



