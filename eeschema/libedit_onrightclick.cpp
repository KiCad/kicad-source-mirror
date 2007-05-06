	/****************************/
	/*	EESchema - libedit_onrightclick.cpp	*/
	/****************************/

/* , In library editor, create the pop menu when clicking on mouse right button
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "bitmaps.h"
#include "protos.h"

#include "id.h"

#include "Pin_to.xpm"
#include "Pin_Size_to.xpm"
#include "Pin_Name_to.xpm"
#include "Pin_Number_to.xpm"
#include "Delete_Pin.xpm"

/* functions to add commands and submenus depending on the item */
static void AddMenusForBlock(wxMenu * PopMenu, WinEDA_LibeditFrame * frame);
static void AddMenusForPin(wxMenu * PopMenu, LibDrawPin* Pin, WinEDA_LibeditFrame * frame);


/********************************************************************************/
void WinEDA_LibeditFrame::OnRightClick(const wxPoint& MousePos, wxMenu * PopMenu)
/********************************************************************************/
{
LibEDA_BaseStruct* DrawEntry = CurrentDrawItem;
bool BlockActive = (m_CurrentScreen->BlockLocate.m_Command !=  BLOCK_IDLE);

	if ( CurrentLibEntry == NULL ) return;

	if ( (DrawEntry == NULL) || (DrawEntry->m_Flags == 0) )
	{ 	// Simple localisation des elements
		DrawEntry = LocatePin(m_CurrentScreen->m_Curseur, CurrentLibEntry, CurrentUnit, CurrentConvert);
		if ( DrawEntry == NULL )
		{
			DrawEntry = CurrentDrawItem = LocateDrawItem(GetScreen(), CurrentLibEntry,CurrentUnit,
					CurrentConvert,LOCATE_ALL_DRAW_ITEM);
		}
		if ( DrawEntry == NULL )
		{
			DrawEntry = CurrentDrawItem = (LibEDA_BaseStruct*)
				LocateField(CurrentLibEntry);
		}
	}

	//  If Command in progresss: put the menu "cancel" and "end tool"
	if ( m_ID_current_state	)
	{
		if (DrawEntry && DrawEntry->m_Flags)
		{
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING,
				_("Cancel"), cancel_xpm);
		}
		else
		{
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING,
				_("End Tool"), cancel_tool_xpm);
		}
		PopMenu->AppendSeparator();
	}

	else
	{
		if ( (DrawEntry && DrawEntry->m_Flags) || BlockActive )
		{
			if ( BlockActive ) AddMenusForBlock( PopMenu, this);
			else ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _("Cancel"), cancel_xpm);
			PopMenu->AppendSeparator();
		}
	}

	if ( DrawEntry ) DrawEntry->Display_Infos_DrawEntry(this);
	else return;

	CurrentDrawItem = DrawEntry;

	switch ( DrawEntry->m_StructType )
	{
		case  COMPONENT_PIN_DRAW_TYPE:
			AddMenusForPin(PopMenu, (LibDrawPin*)DrawEntry, this);
			break;

		case COMPONENT_ARC_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Arc"), move_arc_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM,
					_("Arc Options"), options_arc_xpm );
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
					_("Arc Delete"), delete_arc_xpm);
			}
			break;

		case COMPONENT_CIRCLE_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Circle"), move_circle_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM,
				_("Circle Options"), options_circle_xpm);
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
					_("Circle Delete"), delete_circle_xpm);
			}
			break;

		case COMPONENT_RECT_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Rect"), move_rectangle_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM,
				_("Rect Options"), options_rectangle_xpm);
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
					_("Rect Delete"), delete_rectangle_xpm);
			}
			break;

		case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Text"), move_text_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM,
				_("Text Editor"), edit_text_xpm);
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT,
				_("Rotate Text"), edit_text_xpm);
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
					_("Text Delete"), delete_text_xpm);
			}
			break;

		case COMPONENT_POLYLINE_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Line"), move_line_xpm);
			}
			if ( DrawEntry->m_Flags & IS_NEW )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_END_CREATE_ITEM,
					_("Line End"), apply_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_BODY_EDIT_ITEM,
				_("Line Options"), options_segment_xpm);
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
					_("Line Delete"), delete_segment_xpm);
			}
			else if( (DrawEntry->m_Flags & IS_NEW) )
			{
				if( ((LibDrawPolyline*)DrawEntry)->n > 2 )
					ADD_MENUITEM(PopMenu,
						ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
						_("Segment Delete"), delete_segment_xpm);
			}	
			break;

		case COMPONENT_FIELD_DRAW_TYPE:
			if( DrawEntry->m_Flags == 0 )
			{
				ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST,
					_("Move Field"), move_field_xpm);
			}
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM,
				_("Field Rotate"), rotate_field_xpm);
			ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM,
				_("Field Edit"), edit_text_xpm);
			break;


		default:
			wxString msg;
			msg.Printf(
				 wxT("WinEDA_LibeditFrame::OnRightClick Error: unknown StructType %d"),
				DrawEntry->m_StructType);
			DisplayError(this, msg );
			CurrentDrawItem = NULL;
			break;
	}
	PopMenu->AppendSeparator();
}

/**********************************************************************************/
void AddMenusForPin(wxMenu * PopMenu, LibDrawPin* Pin, WinEDA_LibeditFrame * frame)
/**********************************************************************************/
{
bool selected = (Pin->m_Selected & IS_SELECTED) != 0;
bool not_in_move = (Pin->m_Flags == 0);

	if( not_in_move )	
		ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST, _("Move Pin"), move_xpm );

	ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_PIN_EDIT, _("Pin Edit"), edit_xpm );

	if( not_in_move )
	{
		ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_DELETE_ITEM,
			_("Pin Delete"), delete_pin_xpm );
	}
	wxMenu * global_pin_change = new wxMenu;
	ADD_MENUITEM_WITH_SUBMENU(PopMenu, global_pin_change, ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
				_("Global"), pin_to_xpm);
	ADD_MENUITEM(global_pin_change, ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM,
				selected ? _("Pin Size to selected pins"): _("Pin Size to others"),
				pin_size_to_xpm);
	ADD_MENUITEM(global_pin_change, ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM,
				selected ? _("Pin Name Size to selected pin") :  _("Pin Name Size to others"),
				pin_name_to_xpm);
	ADD_MENUITEM(global_pin_change, ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM,
				selected ?_("Pin Num Size to selected pin") :  _("Pin Num Size to others"),
				pin_number_to_xpm);
}


/**********************************************************************/
void AddMenusForBlock(wxMenu * PopMenu, WinEDA_LibeditFrame * frame)
/**********************************************************************/
/* Add menu commands for block
*/
{
	ADD_MENUITEM(PopMenu, ID_POPUP_LIBEDIT_CANCEL_EDITING, _("Cancel Block"), cancel_xpm);

	if ( frame->GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
		ADD_MENUITEM(PopMenu, ID_POPUP_ZOOM_BLOCK,
			_("Win. Zoom (Midd butt drag mouse)"), zoom_selected_xpm);

	PopMenu->AppendSeparator();

	ADD_MENUITEM(PopMenu, ID_POPUP_PLACE_BLOCK, _("Place Block"), apply_xpm );

	if ( frame->GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
	{
		ADD_MENUITEM(PopMenu, ID_POPUP_SELECT_ITEMS_BLOCK, _("Select items"), green_xpm);
		ADD_MENUITEM(PopMenu, ID_POPUP_COPY_BLOCK,
			_("Copy Block (shift + drag mouse)"), copyblock_xpm);
		ADD_MENUITEM(PopMenu, ID_POPUP_INVERT_BLOCK, _("Mirror Block (ctrl + drag mouse)"), mirror_H_xpm );
		ADD_MENUITEM(PopMenu, ID_POPUP_DELETE_BLOCK,
			_("Del. Block (shift+ctrl + drag mouse)"), delete_xpm );
	}
}

