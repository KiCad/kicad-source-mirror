	/******************************************************/
	/* schedit.cpp: fonctions generales de la schematique */
	/******************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"

/********************************************************************************/
void WinEDA_SchematicFrame::Process_Special_Functions(wxCommandEvent& event)
/********************************************************************************/
/* Traite les selections d'outils et les commandes appelees du menu POPUP
*/
{
int id = event.GetId();
wxPoint pos;
wxClientDC dc(DrawPanel);
wxPoint defaultpos(-1,-1);

	DrawPanel->PrepareGraphicContext(&dc);

	pos = wxGetMousePosition();

	pos.y += 20;

	// If needed, stop the current command and deselect current tool
	switch ( id )
	{
		case ID_POPUP_SCH_ENTRY_SELECT_SLASH:	// Do nothing:
		case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
		case ID_POPUP_END_LINE:
		case ID_POPUP_SCH_EDIT_TEXT:
		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
		case ID_POPUP_SCH_SET_SHAPE_TEXT:
		case ID_POPUP_SCH_ROTATE_TEXT:
		case ID_POPUP_SCH_EDIT_SHEET:
		case ID_POPUP_SCH_CLEANUP_SHEET:
		case ID_POPUP_SCH_END_SHEET:
		case ID_POPUP_SCH_RESIZE_SHEET:
		case ID_POPUP_SCH_EDIT_PINSHEET:
		case ID_POPUP_SCH_MOVE_PINSHEET:
		case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
		case ID_POPUP_SCH_MOVE_CMP_REQUEST:
		case ID_POPUP_SCH_EDIT_CMP:
		case ID_POPUP_SCH_MIROR_X_CMP:
		case ID_POPUP_SCH_MIROR_Y_CMP:
		case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
		case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
		case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
		case ID_POPUP_SCH_INIT_CMP:
		case ID_POPUP_SCH_DISPLAYDOC_CMP:
		case ID_POPUP_SCH_EDIT_VALUE_CMP:
		case ID_POPUP_SCH_EDIT_REF_CMP:
		case ID_POPUP_SCH_EDIT_CONVERT_CMP:
		case ID_POPUP_SCH_SELECT_UNIT_CMP:
		case ID_POPUP_SCH_SELECT_UNIT1:
		case ID_POPUP_SCH_SELECT_UNIT2:
		case ID_POPUP_SCH_SELECT_UNIT3:
		case ID_POPUP_SCH_SELECT_UNIT4:
		case ID_POPUP_SCH_SELECT_UNIT5:
		case ID_POPUP_SCH_SELECT_UNIT6:
		case ID_POPUP_SCH_SELECT_UNIT7:
		case ID_POPUP_SCH_SELECT_UNIT8:
		case ID_POPUP_SCH_SELECT_UNIT9:
		case ID_POPUP_SCH_SELECT_UNIT10:
		case ID_POPUP_SCH_SELECT_UNIT11:
		case ID_POPUP_SCH_SELECT_UNIT12:
		case ID_POPUP_SCH_SELECT_UNIT13:
		case ID_POPUP_SCH_SELECT_UNIT14:
		case ID_POPUP_SCH_SELECT_UNIT15:
		case ID_POPUP_SCH_SELECT_UNIT16:
		case ID_POPUP_SCH_SELECT_UNIT17:
		case ID_POPUP_SCH_SELECT_UNIT18:
		case ID_POPUP_SCH_SELECT_UNIT19:
		case ID_POPUP_SCH_SELECT_UNIT20:
		case ID_POPUP_SCH_SELECT_UNIT21:
		case ID_POPUP_SCH_SELECT_UNIT22:
		case ID_POPUP_SCH_SELECT_UNIT23:
		case ID_POPUP_SCH_SELECT_UNIT24:
		case ID_POPUP_SCH_SELECT_UNIT25:
		case ID_POPUP_SCH_SELECT_UNIT26:
		case ID_POPUP_SCH_ROTATE_FIELD:
		case ID_POPUP_SCH_EDIT_FIELD:
		case ID_POPUP_DELETE_BLOCK:
		case ID_POPUP_PLACE_BLOCK:
		case ID_POPUP_ZOOM_BLOCK:
		case ID_POPUP_DRAG_BLOCK:
		case ID_POPUP_COPY_BLOCK:
		case ID_POPUP_ROTATE_BLOCK:
		case ID_POPUP_MIRROR_X_BLOCK:
		case ID_POPUP_MIRROR_Y_BLOCK:
		case ID_POPUP_SCH_DELETE_NODE:
		case ID_POPUP_SCH_DELETE_CONNECTION:
		case wxID_CUT:
		case wxID_COPY:
		case ID_POPUP_SCH_ENTER_SHEET:
		case ID_POPUP_SCH_LEAVE_SHEET:
		case ID_POPUP_SCH_ADD_JUNCTION:
		case ID_POPUP_SCH_ADD_LABEL:
			break;		// Do nothing:

		case ID_POPUP_CANCEL_CURRENT_COMMAND:
			if (m_CurrentScreen->BlockLocate.m_Command != BLOCK_IDLE)
				DrawPanel->SetCursor(wxCursor(DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor) );

			if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
			{
				DrawPanel->ForceCloseManageCurseur(DrawPanel, &dc);
			}
			/* ne devrait pas etre execute, sauf bug: */
			if (m_CurrentScreen->BlockLocate.m_Command != BLOCK_IDLE)
			{
				m_CurrentScreen->BlockLocate.m_Command = BLOCK_IDLE;
				m_CurrentScreen->BlockLocate.m_State = STATE_NO_BLOCK;
				m_CurrentScreen->BlockLocate.m_BlockDrawStruct = NULL;
			}
			break;

		case ID_POPUP_SCH_DELETE_CMP:
		case ID_POPUP_SCH_DELETE:	// Stop the, current command, keep the current tool
			if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
			{
				DrawPanel->ForceCloseManageCurseur(DrawPanel, &dc);
			}
			break;

		default:	// Stop the current command, and deselect the current tool
			if(DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
			{
				DrawPanel->ForceCloseManageCurseur(DrawPanel, &dc);
			}
			DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor = wxCURSOR_ARROW;
			SetToolID(0, DrawPanel->m_PanelCursor, wxEmptyString);
			break;
	}	// End switch commande en cours

	switch ( id )	// Command execution:
	{
		case ID_EXIT :
			Close(TRUE);
			break;

		case ID_NEW_PROJECT: /* New EED Project */
			LoadOneEEProject( wxEmptyString, TRUE);
			break;

		case ID_LOAD_PROJECT:
			LoadOneEEProject( wxEmptyString, FALSE);
			break;

		case ID_LOAD_ONE_SHEET:
			LoadOneSheet(GetScreen(), wxEmptyString);
			break;

		case ID_LOAD_FILE_1:
		case ID_LOAD_FILE_2:
		case ID_LOAD_FILE_3:
		case ID_LOAD_FILE_4:
		case ID_LOAD_FILE_5:
		case ID_LOAD_FILE_6:
		case ID_LOAD_FILE_7:
		case ID_LOAD_FILE_8:
		case ID_LOAD_FILE_9:
		case ID_LOAD_FILE_10:
			LoadOneEEProject(GetLastProject(id - ID_LOAD_FILE_1).GetData(), FALSE);
			break;

		case ID_TO_LIBRARY :
			if ( m_Parent->LibeditFrame )
				{
				m_Parent->LibeditFrame->Show(TRUE);
				}
			else
				{
				m_Parent->LibeditFrame = new
							WinEDA_LibeditFrame(m_Parent->SchematicFrame,
							m_Parent,
							 wxT("Library Editor"),
							wxPoint(-1,-1), wxSize(600,400) );
				ActiveScreen = ScreenLib;
				m_Parent->LibeditFrame->AdjustScrollBars();
				}
			break;

		case ID_TO_PCB:
		{
			wxString Line;
			if( ScreenSch->m_FileName != wxEmptyString )
			{
				Line = ScreenSch->m_FileName;
				AddDelimiterString(Line);
				ChangeFileNameExt( Line,wxEmptyString);
				ExecuteFile(this, PCBNEW_EXE, Line);
			}

			else ExecuteFile(this, PCBNEW_EXE);
			break;
		}

		case ID_TO_CVPCB:
			{
			wxString Line;
			if( ScreenSch->m_FileName != wxEmptyString )
				{
				Line = ScreenSch->m_FileName;
				AddDelimiterString(Line);
				ChangeFileNameExt( Line,wxEmptyString);
				ExecuteFile(this, CVPCB_EXE, Line);
				}

			else ExecuteFile(this, CVPCB_EXE);
			break;
			}

		case ID_TO_LIBVIEW :
			if ( m_Parent->ViewlibFrame )
				{
				m_Parent->ViewlibFrame->Show(TRUE);
				}
			else
				{
				m_Parent->ViewlibFrame = new
							WinEDA_ViewlibFrame(m_Parent->SchematicFrame, m_Parent);
				m_Parent->ViewlibFrame->AdjustScrollBars();
				}
			break;

		case ID_HIERARCHY:
			InstallHierarchyFrame(&dc, pos);
			g_ItemToRepeat = NULL;
			break;

		case wxID_CUT:
			if ( m_CurrentScreen->BlockLocate.m_Command != BLOCK_MOVE )
				break;
			HandleBlockEndByPopUp(BLOCK_DELETE, &dc);
			g_ItemToRepeat = NULL;
			break;


		case wxID_PASTE:
			HandleBlockBegin(&dc, BLOCK_PASTE,m_CurrentScreen->m_Curseur);
			break;

		case ID_GET_ANNOTATE:
			InstallAnnotateFrame(this, defaultpos);
			break;

		case ID_GET_ERC:
			InstallErcFrame(this, defaultpos);
			break;

		case ID_GET_NETLIST:
			InstallNetlistFrame(this, defaultpos);
			break;

		case ID_GET_TOOLS:
			InstallToolsFrame(this, defaultpos );
			break;

		case ID_FIND_ITEMS:
			InstallFindFrame(this, pos);
			break;

		case ID_HIERARCHY_PUSH_POP_BUTT:
			SetToolID( id, wxCURSOR_HAND, _("Push/Pop Hierarchy") );
			break;

		case ID_NOCONN_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add NoConnect Flag"));
			break;

		case ID_WIRE_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Wire"));
			break;

		case ID_BUS_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Bus"));
			break;

		case ID_LINE_COMMENT_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Drawing"));
			break;

		case ID_JUNCTION_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Junction"));
			break;

		case ID_LABEL_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Label"));
			break;

		case ID_GLABEL_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Global label"));
			break;

		case ID_TEXT_COMMENT_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Text"));
			break;

		case ID_WIRETOBUS_ENTRY_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Wire to Bus Entry"));
			break;

		case ID_BUSTOBUS_ENTRY_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Bus to Bus entry"));
			break;

		case ID_SHEET_SYMBOL_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Sheet"));
			break;

		case ID_SHEET_LABEL_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add PinSheet"));
			break;

		case ID_IMPORT_GLABEL_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Import PinSheet"));
			break;

		case ID_COMPONENT_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Component"));
			break;

		case ID_PLACE_POWER_BUTT:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Power"));
			break;

		case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
			DrawPanel->MouseToCursorSchema();
			SetBusEntryShape(&dc,
				(DrawBusEntryStruct*)m_CurrentScreen->m_CurrentItem, '/');
			break;

		case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
			DrawPanel->MouseToCursorSchema();
			SetBusEntryShape(&dc,
				(DrawBusEntryStruct*)m_CurrentScreen->m_CurrentItem, '\\');
			break;

		case ID_NO_SELECT_BUTT:
			SetToolID( 0, wxCURSOR_ARROW, wxEmptyString);
			break;

		case ID_POPUP_CANCEL_CURRENT_COMMAND:
			if (m_ID_current_state == 0)
				SetToolID( 0, wxCURSOR_ARROW, wxEmptyString);
			break;

		case ID_POPUP_END_LINE:
			DrawPanel->MouseToCursorSchema();
			EndSegment(&dc);
			break;

		case ID_POPUP_SCH_EDIT_TEXT:
			EditSchematicText(
				(DrawTextStruct*)m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_ROTATE_TEXT:
			DrawPanel->MouseToCursorSchema();
			ChangeTextOrient(
					(DrawTextStruct*)m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
			DrawPanel->MouseToCursorSchema();
			ConvertTextType( (DrawTextStruct*)m_CurrentScreen->m_CurrentItem,
						&dc, DRAW_LABEL_STRUCT_TYPE);
			break;

		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
			DrawPanel->MouseToCursorSchema();
			ConvertTextType( (DrawTextStruct*)m_CurrentScreen->m_CurrentItem,
						&dc, DRAW_GLOBAL_LABEL_STRUCT_TYPE);
			break;

		case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
			DrawPanel->MouseToCursorSchema();
			ConvertTextType( (DrawTextStruct*)m_CurrentScreen->m_CurrentItem,
						&dc, DRAW_TEXT_STRUCT_TYPE);
			break;

		case ID_POPUP_SCH_SET_SHAPE_TEXT:
			// Non utilisé
			break;

		case ID_POPUP_SCH_ROTATE_FIELD:
			DrawPanel->MouseToCursorSchema();
			RotateCmpField( (PartTextStruct *)m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_EDIT_FIELD:
			EditCmpFieldText( (PartTextStruct *)m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_DELETE_NODE:
		case ID_POPUP_SCH_DELETE_CONNECTION:
			DrawPanel->MouseToCursorSchema();
			DeleteConnection(&dc, id == ID_POPUP_SCH_DELETE_CONNECTION ? TRUE : FALSE);
			m_CurrentScreen->m_CurrentItem = NULL;
			g_ItemToRepeat = NULL;
			TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
			break;

		case ID_POPUP_SCH_BREAK_WIRE:
			{
			DrawPickedStruct * ListForUndo;
			DrawPanel->MouseToCursorSchema();
			ListForUndo = BreakSegment((SCH_SCREEN*)m_CurrentScreen,
					m_CurrentScreen->m_Curseur, TRUE);
			if ( ListForUndo ) SaveCopyInUndoList(ListForUndo, IS_NEW|IS_CHANGED);
			TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
			}
			break;

		case ID_POPUP_SCH_DELETE_CMP:
			if ( m_CurrentScreen->m_CurrentItem == NULL) break;
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
		case ID_POPUP_SCH_DELETE:
			if ( m_CurrentScreen->m_CurrentItem == NULL) break;
			DeleteStruct(this->DrawPanel, &dc, m_CurrentScreen->m_CurrentItem);
			m_CurrentScreen->m_CurrentItem = NULL;
			g_ItemToRepeat = NULL;
			TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
			m_CurrentScreen->SetModify();
			break;

		case ID_SCHEMATIC_DELETE_ITEM_BUTT:
			SetToolID( id, wxCURSOR_BULLSEYE, _("Delete item"));
			break;

		case ID_POPUP_SCH_END_SHEET:
			DrawPanel->MouseToCursorSchema();
			m_CurrentScreen->m_CurrentItem->Place(this, &dc);
			break;

		case ID_POPUP_SCH_RESIZE_SHEET:
			DrawPanel->MouseToCursorSchema();
			ReSizeSheet((DrawSheetStruct *)
				m_CurrentScreen->m_CurrentItem, &dc);
			TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
			break;

		case ID_POPUP_SCH_EDIT_SHEET:
			EditSheet((DrawSheetStruct *)
				m_CurrentScreen->m_CurrentItem, &dc);
			break;
		case ID_POPUP_SCH_CLEANUP_SHEET:
			((DrawSheetStruct *)
				m_CurrentScreen->m_CurrentItem)->CleanupSheet(this, &dc);
			break;

		case ID_POPUP_SCH_EDIT_PINSHEET:
			Edit_PinSheet((DrawSheetLabelStruct *)
				m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_MOVE_PINSHEET:
			DrawPanel->MouseToCursorSchema();
			StartMove_PinSheet((DrawSheetLabelStruct *)
				m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_MOVE_CMP_REQUEST:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
		case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
			DrawPanel->MouseToCursorSchema();
			Process_Move_Item(m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_EDIT_CMP:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			InstallCmpeditFrame(this, pos,
					(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem);
			break;

		case ID_POPUP_SCH_MIROR_X_CMP:
		case ID_POPUP_SCH_MIROR_Y_CMP:
		case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
		case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
		case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
		{
			int option;
			switch (id)
			{
				case ID_POPUP_SCH_MIROR_X_CMP:
					option = CMP_MIROIR_X; break;

				case ID_POPUP_SCH_MIROR_Y_CMP:
					option = CMP_MIROIR_Y; break;

				case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
					option = CMP_ROTATE_COUNTERCLOCKWISE; break;

				case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
					option = CMP_ROTATE_CLOCKWISE; break;

				default:
				case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
					option = CMP_NORMAL; break;
			}
			DrawPanel->MouseToCursorSchema();
			if ( m_CurrentScreen->m_CurrentItem->m_Flags == 0 )
				SaveCopyInUndoList(m_CurrentScreen->m_CurrentItem, IS_CHANGED);
			CmpRotationMiroir(
				(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem,
				&dc, option );
			break;
		}

		case ID_POPUP_SCH_INIT_CMP:
			DrawPanel->MouseToCursorSchema();
			break;

		case ID_POPUP_SCH_EDIT_VALUE_CMP:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			EditComponentValue(
				(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_EDIT_REF_CMP:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			EditComponentReference(
				(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem, &dc);
			break;

		case ID_POPUP_SCH_EDIT_CONVERT_CMP:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			DrawPanel->MouseToCursorSchema();
			ConvertPart(
				(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem,
				&dc);
			break;
			
		case ID_POPUP_SCH_COPY_COMPONENT_CMP:
			DrawPanel->MouseToCursorSchema();
			{
			EDA_SchComponentStruct * olditem, * newitem;
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			olditem = (EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem;
			if ( olditem == NULL ) break;
			newitem = olditem->GenCopy();
			newitem->m_TimeStamp = GetTimeStamp();
			newitem->ClearAnnotation();
			newitem->m_Flags = IS_NEW;
			StartMovePart(newitem, &dc);
			/* Redraw the original part, because StartMovePart() has erase it from screen */
			RedrawOneStruct(DrawPanel, &dc, olditem, GR_DEFAULT_DRAWMODE);
			}
			break;
		
		case ID_POPUP_SCH_SELECT_UNIT1:
		case ID_POPUP_SCH_SELECT_UNIT2:
		case ID_POPUP_SCH_SELECT_UNIT3:
		case ID_POPUP_SCH_SELECT_UNIT4:
		case ID_POPUP_SCH_SELECT_UNIT5:
		case ID_POPUP_SCH_SELECT_UNIT6:
		case ID_POPUP_SCH_SELECT_UNIT7:
		case ID_POPUP_SCH_SELECT_UNIT8:
		case ID_POPUP_SCH_SELECT_UNIT9:
		case ID_POPUP_SCH_SELECT_UNIT10:
		case ID_POPUP_SCH_SELECT_UNIT11:
		case ID_POPUP_SCH_SELECT_UNIT12:
		case ID_POPUP_SCH_SELECT_UNIT13:
		case ID_POPUP_SCH_SELECT_UNIT14:
		case ID_POPUP_SCH_SELECT_UNIT15:
		case ID_POPUP_SCH_SELECT_UNIT16:
		case ID_POPUP_SCH_SELECT_UNIT17:
		case ID_POPUP_SCH_SELECT_UNIT18:
		case ID_POPUP_SCH_SELECT_UNIT19:
		case ID_POPUP_SCH_SELECT_UNIT20:
		case ID_POPUP_SCH_SELECT_UNIT21:
		case ID_POPUP_SCH_SELECT_UNIT22:
		case ID_POPUP_SCH_SELECT_UNIT23:
		case ID_POPUP_SCH_SELECT_UNIT24:
		case ID_POPUP_SCH_SELECT_UNIT25:
		case ID_POPUP_SCH_SELECT_UNIT26:
			// Ensure the struct is a component (could be a struct of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			DrawPanel->MouseToCursorSchema();
			SelPartUnit(
				(EDA_SchComponentStruct *) m_CurrentScreen->m_CurrentItem,
				id + 1 - ID_POPUP_SCH_SELECT_UNIT1,
				&dc);
			break;

		case ID_POPUP_SCH_DISPLAYDOC_CMP:
			// Ensure the struct is a component (could be a piece of a component, like Field, text..)
			if ( m_CurrentScreen->m_CurrentItem->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				m_CurrentScreen->m_CurrentItem = LocateSmallestComponent( GetScreen() );
			if ( m_CurrentScreen->m_CurrentItem == NULL ) break;
			{
			EDA_LibComponentStruct * LibEntry;
			LibEntry = FindLibPart(((EDA_SchComponentStruct *)
							m_CurrentScreen->m_CurrentItem)->m_ChipName,
							wxEmptyString, FIND_ALIAS);
			if ( LibEntry && LibEntry->m_DocFile != wxEmptyString )
				GetAssociatedDocument(this, g_RealLibDirBuffer, LibEntry->m_DocFile);
			}
      	break;

		case ID_POPUP_SCH_ENTER_SHEET:
			{
			EDA_BaseStruct *DrawStruct = m_CurrentScreen->m_CurrentItem;
			if ( DrawStruct && (DrawStruct->m_StructType == DRAW_SHEET_STRUCT_TYPE) )
				{
				InstallNextScreen((DrawSheetStruct *) DrawStruct);
				}
			}
			break;

		case ID_POPUP_SCH_LEAVE_SHEET:
			InstallPreviousScreen();
			break;

		case ID_POPUP_CLOSE_CURRENT_TOOL:
			SetToolID( 0, wxCURSOR_ARROW, wxEmptyString);
			break;

		case wxID_COPY:		// really this is a Save block for paste
			HandleBlockEndByPopUp(BLOCK_SAVE, &dc);
			break;

		case ID_POPUP_PLACE_BLOCK:
			DrawPanel->m_AutoPAN_Request = FALSE;
			DrawPanel->MouseToCursorSchema();
			HandleBlockPlace(&dc);
			break;

		case ID_POPUP_ZOOM_BLOCK:
			HandleBlockEndByPopUp(BLOCK_ZOOM, &dc);
			break;

		case ID_POPUP_DELETE_BLOCK:
			DrawPanel->MouseToCursorSchema();
			HandleBlockEndByPopUp(BLOCK_DELETE, &dc);
			break;

		case ID_POPUP_ROTATE_BLOCK:
			DrawPanel->MouseToCursorSchema();
			HandleBlockEndByPopUp(BLOCK_ROTATE, &dc);
			break;

		case ID_POPUP_MIRROR_X_BLOCK:
		case ID_POPUP_MIRROR_Y_BLOCK:
			DrawPanel->MouseToCursorSchema();
			HandleBlockEndByPopUp(BLOCK_MIRROR_Y, &dc);
			break;

		case ID_POPUP_COPY_BLOCK:
			DrawPanel->MouseToCursorSchema();
			HandleBlockEndByPopUp(BLOCK_COPY, &dc);
			break;

		case ID_POPUP_DRAG_BLOCK:
			DrawPanel->MouseToCursorSchema();
			HandleBlockEndByPopUp(BLOCK_DRAG, &dc);
			break;

		case ID_POPUP_SCH_ADD_JUNCTION:
			DrawPanel->MouseToCursorSchema();
			m_CurrentScreen->m_CurrentItem = 
				CreateNewJunctionStruct(&dc, m_CurrentScreen->m_Curseur, TRUE);
			TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
			m_CurrentScreen->m_CurrentItem = NULL;
			break;
		
		case ID_POPUP_SCH_ADD_LABEL:
		case ID_POPUP_SCH_ADD_GLABEL:
			m_CurrentScreen->m_CurrentItem = CreateNewText(&dc,
				id == ID_POPUP_SCH_ADD_LABEL ? LAYER_LOCLABEL : LAYER_GLOBLABEL);
			if ( m_CurrentScreen->m_CurrentItem )
			{
				m_CurrentScreen->m_CurrentItem->Place( this, &dc);
				TestDanglingEnds(m_CurrentScreen->EEDrawList, &dc);
				m_CurrentScreen->m_CurrentItem = NULL;
			}
			break;

		case ID_SCHEMATIC_UNDO:
			GetSchematicFromUndoList();
			DrawPanel->Refresh(TRUE);
			break;

		case ID_SCHEMATIC_REDO:
			GetSchematicFromRedoList();
			DrawPanel->Refresh(TRUE);
			break;

		default:	// Log error:
			DisplayError(this, wxT("WinEDA_SchematicFrame::Process_Special_Functions error") );
			break;
	}	// End switch ( id )	(Command execution)

	if ( m_ID_current_state == 0 ) g_ItemToRepeat = NULL;
	SetToolbars();

	dc.SetBrush(wxNullBrush);
	dc.SetPen(wxNullPen);
}


/********************************************************************************/
void WinEDA_SchematicFrame::Process_Move_Item(EDA_BaseStruct *DrawStruct,
					wxDC * DC)
/********************************************************************************/
{

	if ( DrawStruct == NULL ) return;

	DrawPanel->MouseToCursorSchema();
	switch ( DrawStruct->m_StructType )
		{
		case DRAW_JUNCTION_STRUCT_TYPE:
			break;

		case DRAW_BUSENTRY_STRUCT_TYPE:
			StartMoveBusEntry((DrawBusEntryStruct *) DrawStruct, DC);
			break;

		case DRAW_LABEL_STRUCT_TYPE:
		case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
		case DRAW_TEXT_STRUCT_TYPE:
			StartMoveTexte( (DrawTextStruct *) DrawStruct, DC);
			break;

		case DRAW_LIB_ITEM_STRUCT_TYPE:
			StartMovePart( (EDA_SchComponentStruct *) DrawStruct, DC);
			break;

		case DRAW_SEGMENT_STRUCT_TYPE:
			break;

		case DRAW_SHEET_STRUCT_TYPE:
			StartMoveSheet( (DrawSheetStruct*) DrawStruct, DC);
			break;

		case DRAW_NOCONNECT_STRUCT_TYPE:
			break;

		case DRAW_PART_TEXT_STRUCT_TYPE:
			StartMoveCmpField( (PartTextStruct *) DrawStruct, DC);
			break;

		case DRAW_MARKER_STRUCT_TYPE:
		case DRAW_SHEETLABEL_STRUCT_TYPE:
		default:
			wxString msg;
			msg.Printf(
				 wxT("WinEDA_SchematicFrame::Move_Item Error: Bad DrawType %d"),
				DrawStruct->m_StructType);
			DisplayError(this, msg );
			break;
		}
}


