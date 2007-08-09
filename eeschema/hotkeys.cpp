	/***************/
	/* hotkeys.cpp */
	/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"

/* Routines locales */

/* variables externes */

/***********************************************************/
void WinEDA_SchematicFrame::OnHotKey(wxDC * DC, int hotkey,
					EDA_BaseStruct * DrawStruct)
/***********************************************************/
/* Hot keys. Some commands are relatives to the item under the mouse cursor
	Commands are case insensitive
	Zoom commands are not managed here
*/
/* Hotkey list: */
{
static wxString s_Hotkey_List[] =
{
	wxT("key F1: Zoom in"),			// general zoom hotkey, not managed here
	wxT("key F2: Zoom out"),		// general zoom hotkey, not managed here
	wxT("key F5: Zoom Redraw"),		// general zoom hotkey, not managed here
	wxT("key F4: Zoom Center"),		// general zoom hotkey, not managed here
	wxT("key F5: Next search"),
	wxT("key DELETE: delete item"),
	wxT("key R:  Rotation (component or label)"),
	wxT("key X:  Mirror X (component)"),
	wxT("key Y:  Mirror Y (component)"),
	wxT("key N:  Orient 0 (component)"),
	wxT("key M:  Start Move component"),
	wxT("key A:  Add new component"),
	wxT("key W:  begin new Wire"),
	wxT("")							// End of list, do not change
};


bool PopupOn = m_CurrentScreen->m_CurrentItem  &&
			m_CurrentScreen->m_CurrentItem->m_Flags;
bool RefreshToolBar = FALSE;	// We must refresh tool bar when the undo/redo tool state is modified
	
	if ( hotkey == 0 ) return;

wxPoint MousePos = m_CurrentScreen->m_MousePosition; 

	switch (hotkey)
	{
		case '?':	// Display Current hotkey list
		{
			wxString msg = _("Current hotkey list:\n\n");
			for ( unsigned int ii = 0; ; ii++ )
			{
				if ( s_Hotkey_List[ii].IsEmpty() ) break;
				msg += s_Hotkey_List[ii]; msg += wxT("\n");
			}
			DisplayInfo(this, msg);
			break;
		}

		case WXK_DELETE:
		case WXK_NUMPAD_DELETE:
			if ( PopupOn ) break;
			RefreshToolBar = LocateAndDeleteItem(this, DC);
			m_CurrentScreen->SetModify();
			m_CurrentScreen->m_CurrentItem = NULL;
			TestDanglingEnds(m_CurrentScreen->EEDrawList, DC);
			break;

		case WXK_F5 :
			if ( g_LastSearchIsMarker ) WinEDA_SchematicFrame::FindMarker(1);
			else FindSchematicItem(wxEmptyString, 2);
			break;

        case 'a':
        case 'A':	// Add component
			if ( DrawStruct && DrawStruct->m_Flags ) break;
            // switch to m_ID_current_state = ID_COMPONENT_BUTT;
            if ( m_ID_current_state != ID_COMPONENT_BUTT ) SetToolID( ID_COMPONENT_BUTT, wxCURSOR_PENCIL, _("Add Component"));
            OnLeftClick(DC, MousePos);
			break;

        case 'w':
        case 'W':	// Add wire
 			if ( DrawStruct )	// An item is selected. If edited and not a wire, a new command is not possible
			{
				if ( DrawStruct->m_Flags )	// Item selected and edition in progress
				{
					if (DrawStruct->m_StructType == DRAW_SEGMENT_STRUCT_TYPE )
					{
						EDA_DrawLineStruct * segment = (EDA_DrawLineStruct *)DrawStruct;
						if ( segment->m_Layer != LAYER_WIRE ) break;
					}
					else break;
				}
			}
           // switch to m_ID_current_state = ID_WIRE_BUTT;
            if ( m_ID_current_state != ID_WIRE_BUTT ) SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _("Add Wire"));
            OnLeftClick(DC, MousePos);
            break;

		case 'r':	// Rotation
		case 'R':
			if ( DrawStruct == NULL )
			{
				DrawStruct = PickStruct( GetScreen()->m_Curseur,
					GetScreen()->EEDrawList, LIBITEM|TEXTITEM|LABELITEM );
				if ( DrawStruct == NULL ) break;
				if ( DrawStruct->m_StructType == DRAW_LIB_ITEM_STRUCT_TYPE )
						DrawStruct = LocateSmallestComponent( GetScreen() );
				if ( DrawStruct == NULL ) break;
			}
			switch (DrawStruct->m_StructType)
			{
				case DRAW_LIB_ITEM_STRUCT_TYPE:
					if ( DrawStruct->m_Flags == 0 )
					{
						SaveCopyInUndoList(DrawStruct, IS_CHANGED);
						RefreshToolBar = TRUE;
					}
						
					CmpRotationMiroir(
						(EDA_SchComponentStruct *) DrawStruct, DC, CMP_ROTATE_COUNTERCLOCKWISE );
					break;

				case DRAW_TEXT_STRUCT_TYPE:
				case DRAW_LABEL_STRUCT_TYPE:
				case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
					if ( DrawStruct->m_Flags == 0 )
					{
						SaveCopyInUndoList(DrawStruct, IS_CHANGED);
						RefreshToolBar = TRUE;
					}
						ChangeTextOrient( (DrawTextStruct*)DrawStruct, DC);
						break;
			}
			break;

		case 'y':	// Mirror Y (drawlibpart)
		case 'Y':
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				if ( DrawStruct->m_Flags == 0 )
				{
					SaveCopyInUndoList(DrawStruct, IS_CHANGED);
					RefreshToolBar = TRUE;
				}
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_MIROIR_Y );
			}
			break;

		case 'x':	// Mirror X (drawlibpart)
		case 'X':
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				if ( DrawStruct->m_Flags == 0 )
				{
					SaveCopyInUndoList(DrawStruct, IS_CHANGED);
					RefreshToolBar = TRUE;
				}
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_MIROIR_X );
			}
			break;

		case 'n':
		case 'N':	// Orient 0, no mirror (drawlibpart)
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct )
			{
				if ( DrawStruct->m_Flags == 0 )
				{
					SaveCopyInUndoList(DrawStruct, IS_CHANGED);
					RefreshToolBar = TRUE;
				}
				CmpRotationMiroir(
					(EDA_SchComponentStruct *) DrawStruct, DC, CMP_NORMAL );
				TestDanglingEnds(m_CurrentScreen->EEDrawList, DC);
			}
			break;

		case 'm':
		case 'M':	// Start move drawlibpart
			if ( PopupOn ) break;
			if ( DrawStruct == NULL )
				DrawStruct = LocateSmallestComponent( GetScreen() );
			if ( DrawStruct && (DrawStruct->m_Flags ==0) )
			{
				m_CurrentScreen->m_CurrentItem = DrawStruct;
				Process_Move_Item(m_CurrentScreen->m_CurrentItem, DC);
			}
			break;
	}
	
	if ( RefreshToolBar ) SetToolbars();
}
