/******************************************************************/
/* onrightclick.cpp - creation du menu popup appele par le bouton */
/*	droit de la souris											  */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "id.h"
#include "hotkeys.h"

#include "bitmaps.h"

#include "Enter_Sheet.xpm"
#include "Leave_Sheet.xpm"
#include "Delete_Sheet.xpm"
#include "Resize_Sheet.xpm"
#include "Edit_Sheet.xpm"
#include "Move_Sheet.xpm"
#include "Options_Pinsheet.xpm"
#include "Delete_Pinsheet.xpm"
#include "Delete_Bus.xpm"
#include "Delete_Node.xpm"
#include "Delete_Connection.xpm"
#include "Label2Text.xpm"
#include "Label2GLabel.xpm"
#include "GLabel2Text.xpm"
#include "GLabel2Label.xpm"
#include "Rotate_GLabel.xpm"
#include "GL_Change.xpm"
#include "Edit_Component.xpm"
#include "Break_Line.xpm"
#include "Break_Bus.xpm"
#include "Normal.xpm"
#include "Edit_Comp_Ref.xpm"
#include "Edit_Comp_Value.xpm"

/* functions to add commands and submenus depending on the item */
static void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame );
static void AddMenusForWire( wxMenu* PopMenu, EDA_DrawLineStruct* Wire,
                             WinEDA_SchematicFrame* frame );
static void AddMenusForBus( wxMenu* PopMenu, EDA_DrawLineStruct* Bus,
                            WinEDA_SchematicFrame* frame );
static void AddMenusForHierchicalSheet( wxMenu* PopMenu, DrawSheetStruct* Sheet );
static void AddMenusForPinSheet( wxMenu* PopMenu, DrawSheetLabelStruct* PinSheet );
static void AddMenusForText( wxMenu* PopMenu, DrawTextStruct* Text );
static void AddMenusForLabel( wxMenu* PopMenu, DrawLabelStruct* Label );
static void AddMenusForGLabel( wxMenu* PopMenu, DrawGlobalLabelStruct* GLabel );
static void AddMenusForComponent( wxMenu* PopMenu, EDA_SchComponentStruct* Component );
static void AddMenusForComponentField( wxMenu* PopMenu, PartTextStruct* Field );
static void AddMenusForJunction( wxMenu* PopMenu, DrawJunctionStruct* Junction,
                                 WinEDA_SchematicFrame* frame );


/***********************************************************************/
void WinEDA_SchematicFrame::ToolOnRightClick( wxCommandEvent& event )
/***********************************************************************/
{
    int id = event.GetId();

    switch( id )
    {
    default:
        DisplayError( this, wxT( "ToolOnRightClick() error" ) );
        break;
    }
}


/*****************************************************************/
void WinEDA_SchematicFrame::OnRightClick( const wxPoint& MousePos,
                                          wxMenu*        PopMenu )
/*****************************************************************/

/* Prepare le menu PullUp affiché par un click sur le bouton droit
 *  de la souris.
 *  Ce menu est ensuite complété par la liste des commandes de ZOOM
 */
{
    EDA_BaseStruct* DrawStruct  = m_CurrentScreen->GetCurItem();
    bool            BlockActive = (m_CurrentScreen->BlockLocate.m_Command != BLOCK_IDLE);


    DrawPanel->m_CanStartBlock = -1;    // Ne pas engager un debut de bloc sur validation menu

    // Simple localisation des elements si possible
    if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
    {
        DrawStruct = SchematicGeneralLocateAndDisplay( FALSE );
        if( DrawStruct && (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE) )
        {
            DrawSheetLabelStruct* slabel;
            slabel = LocateSheetLabel( (DrawSheetStruct*) DrawStruct,
                                      m_CurrentScreen->m_Curseur );
            if( slabel )
                DrawStruct = slabel;
        }
    }

    // If Command in progress: put the menu "cancel" and "end tool"
    if(  m_ID_current_state )
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), cancel_xpm );
        }
        else
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL, _( "End Tool" ), cancel_tool_xpm );
        }
        PopMenu->AppendSeparator();
    }
    else
    {
        if( (DrawStruct && DrawStruct->m_Flags) || BlockActive )
        {
            if( BlockActive )
                AddMenusForBlock( PopMenu, this );
            else
                ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), cancel_xpm );
            PopMenu->AppendSeparator();
        }
    }

    if(  BlockActive )
        return;
    if( DrawStruct == NULL )
    {
        if( m_CurrentScreen != ScreenSch )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_LEAVE_SHEET, _( "Leave Sheet" ), leave_sheet_xpm );
            PopMenu->AppendSeparator();
        }
        return;
    }

    m_CurrentScreen->SetCurItem( DrawStruct );

    int  flags  = DrawStruct->m_Flags;
    bool is_new = (flags & IS_NEW) ? TRUE : FALSE;

    switch( DrawStruct->Type() )
    {
    case DRAW_NOCONNECT_STRUCT_TYPE:

//			if( !flags ) PopMenu->Append(ID_POPUP_SCH_MOVE_ITEM_REQUEST, "Move noconnect");
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "delete noconn" ), delete_xpm );
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        AddMenusForJunction( PopMenu, (DrawJunctionStruct*) DrawStruct, this );
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        if( !flags )
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST,
                          _( "Move bus entry" ), move_xpm );
        if( GetBusEntryShape( (DrawBusEntryStruct*) DrawStruct ) == '\\' )
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_SLASH, _( "set bus entry /" ) );
        else
            PopMenu->Append( ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH, _( "set bus entry \\" ) );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                      _( "delete bus entry" ), delete_bus_xpm );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "delete Marker" ), delete_xpm );
        break;

    case DRAW_TEXT_STRUCT_TYPE:
        AddMenusForText( PopMenu, (DrawTextStruct*) DrawStruct );
        break;

    case DRAW_LABEL_STRUCT_TYPE:
        AddMenusForLabel( PopMenu, (DrawLabelStruct*) DrawStruct );
        break;

    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        AddMenusForGLabel( PopMenu, (DrawGlobalLabelStruct*) DrawStruct );
        break;

    case DRAW_PART_TEXT_STRUCT_TYPE:
    {
        AddMenusForComponentField( PopMenu, (PartTextStruct*) DrawStruct );
        if( flags )
            break;

        // Many fields are inside a component. If this is the case, add the component menu
        EDA_SchComponentStruct* Component = LocateSmallestComponent( GetScreen() );
        if( Component )
        {
            PopMenu->AppendSeparator();
            AddMenusForComponent( PopMenu, (EDA_SchComponentStruct*) DrawStruct );
        }
    }
        break;

    case DRAW_LIB_ITEM_STRUCT_TYPE:
        AddMenusForComponent( PopMenu, (EDA_SchComponentStruct*) DrawStruct );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:

//			if( !flags ) PopMenu->Append(ID_POPUP_SCH_MOVE_ITEM_REQUEST, "Move");
        switch( ( (EDA_DrawLineStruct*) DrawStruct )->m_Layer )
        {
        case LAYER_WIRE:
            AddMenusForWire( PopMenu, (EDA_DrawLineStruct*) DrawStruct, this );
            break;

        case LAYER_BUS:
            AddMenusForBus( PopMenu, (EDA_DrawLineStruct*) DrawStruct, this );
            break;

        default:
            if( is_new )
                ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "End drawing" ), apply_xpm );
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                          _( "Delete drawing" ), delete_xpm );
            break;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        AddMenusForHierchicalSheet( PopMenu, (DrawSheetStruct*) DrawStruct );
        break;

    case DRAW_SHEETLABEL_STRUCT_TYPE:
        AddMenusForPinSheet( PopMenu, (DrawSheetLabelStruct*) DrawStruct );
        break;

    default:
        wxString msg;
        msg.Printf(
            wxT( "WinEDA_SchematicFrame::OnRightClick Error: unknown DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();
}


/*************************************************************************/
void AddMenusForComponentField( wxMenu* PopMenu, PartTextStruct* Field )
/*************************************************************************/

/* Add menu commands for a component field (like value, reference)
 */
{
    if( !Field->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, _( "Move Field" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_FIELD, _( "Rotate Field" ), rotate_field_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_FIELD, _( "Edit Field" ), edit_text_xpm );
}


/**************************************************************************/
void AddMenusForComponent( wxMenu* PopMenu, EDA_SchComponentStruct* Component )
/**************************************************************************/

/* Add menu commands for a component
 */
{
    wxString msg;

    EDA_LibComponentStruct* LibEntry;

    LibEntry = FindLibPart( Component->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( !Component->m_Flags )
    {
        msg = AddHotkeyName( _( "Move Component" ), s_Schematic_Hotkey_List, HK_MOVE_COMPONENT );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_CMP_REQUEST,
                      msg, move_xpm );
    }

    // add menu orient et sous menu:
    wxMenu* orientmenu = new wxMenu;
    msg = AddHotkeyName( _( "Rotate +" ), s_Schematic_Hotkey_List, HK_ROTATE_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE,
                  msg, rotate_pos_xpm );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE, _( "Rotate -" ), rotate_neg_xpm );
    msg = AddHotkeyName( _( "Mirror --" ), s_Schematic_Hotkey_List, HK_MIRROR_X_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_X_CMP, msg, mirror_V_xpm );
    msg = AddHotkeyName( _( "Mirror ||" ), s_Schematic_Hotkey_List, HK_MIRROR_Y_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_MIROR_Y_CMP, msg, mirror_H_xpm );
    msg = AddHotkeyName( _( "Normal" ), s_Schematic_Hotkey_List, HK_ORIENT_NORMAL_COMPONENT );
    ADD_MENUITEM( orientmenu, ID_POPUP_SCH_ORIENT_NORMAL_CMP, msg, normal_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, orientmenu,
                               ID_POPUP_SCH_GENERIC_ORIENT_CMP, _(
                                   "Orient Component" ), orient_xpm );

    wxMenu* editmenu = new wxMenu;
    ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_CMP, _( "Edit" ), edit_component_xpm );

    if( LibEntry && LibEntry->m_Options != ENTRY_POWER )
    {
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_VALUE_CMP, _( "Value" ), edit_comp_value_xpm );
        ADD_MENUITEM( editmenu, ID_POPUP_SCH_EDIT_REF_CMP, _( "Reference" ), edit_comp_ref_xpm );
    }
    if( LibEntry && (LookForConvertPart( LibEntry ) >= 2) )
        editmenu->Append( ID_POPUP_SCH_EDIT_CONVERT_CMP, _( "Convert" ) );
    if( LibEntry && (LibEntry->m_UnitCount >= 2) )
    {
        wxMenu* sel_unit_menu = new wxMenu; int ii;
        for( ii = 0; ii < LibEntry->m_UnitCount; ii++ )
        {
            wxString num_unit; num_unit.Printf( _(
                                                    "Unit %d %c" ), ii + 1,
                                                "?ABCDEFGHIJKLMNOPQRSTUVWXYZ"[ii + 1] );
            sel_unit_menu->Append( ID_POPUP_SCH_SELECT_UNIT1 + ii,
                                   num_unit );
        }

        editmenu->Append( ID_POPUP_SCH_SELECT_UNIT_CMP, _( "Unit" ), sel_unit_menu );
    }

    ADD_MENUITEM_WITH_SUBMENU( PopMenu, editmenu,
                               ID_POPUP_SCH_GENERIC_EDIT_CMP, _(
                                   "Edit Component" ), edit_component_xpm );

    if( !Component->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_COPY_COMPONENT_CMP, _( "Copy Component" ), import_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CMP, _( "Delete Component" ), delete_xpm );
    }

    LibEntry = FindLibPart( Component->m_ChipName.GetData(), wxEmptyString, FIND_ALIAS );
    if( LibEntry &&  !LibEntry->m_DocFile.IsEmpty() )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DISPLAYDOC_CMP, _( "Doc" ), datasheet_xpm );
}


/*******************************************************************/
void AddMenusForGLabel( wxMenu* PopMenu, DrawGlobalLabelStruct* GLabel )
/*******************************************************************/

/* Add menu commands for a Global Label
 */
{
    wxMenu* menu_change_type = new wxMenu;

    if( !GLabel->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, _( "Move Glabel" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, _( "Rotate GLabel  (R)" ), rotate_glabel_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, _( "Edit GLabel" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Glabel" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), glabel2label_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), glabel2text_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT, _( "Change Type" ), gl_change_xpm );
}


/*****************************************************************/
void AddMenusForLabel( wxMenu* PopMenu, DrawLabelStruct* Label )
/*****************************************************************/

/* Add menu commands for a Label
 */
{
    wxMenu* menu_change_type = new wxMenu;

    if( !Label->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, _( "Move Label" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, _( "Rotate Label  (R)" ), rotate_pos_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, _( "Edit Label" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Label" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Glabel" ), label2glabel_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT,
                  _( "Change to Text" ), label2text_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT, _( "Change Type" ), gl_change_xpm );
}


/*****************************************************************/
void AddMenusForText( wxMenu* PopMenu, DrawTextStruct* Text )
/*****************************************************************/

/* Add menu commands for a Text (a comment)
 */
{
    wxMenu* menu_change_type = new wxMenu;

    if( !Text->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, _( "Move Text" ), move_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ROTATE_TEXT, _( "Rotate Text (R)" ), rotate_pos_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_TEXT, _( "Edit Text" ), edit_text_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Text" ), delete_text_xpm );

    // add menu change type text (to label, glabel, text):
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL,
                  _( "Change to Label" ), label2text_xpm );
    ADD_MENUITEM( menu_change_type, ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL,
                  _( "Change to Glabel" ), label2glabel_xpm );
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_change_type,
                               ID_POPUP_SCH_CHANGE_TYPE_TEXT, _( "Change Type" ), gl_change_xpm );
}


/*****************************************************************/
void AddMenusForJunction( wxMenu* PopMenu, DrawJunctionStruct* Junction,
                          WinEDA_SchematicFrame* frame )
/*****************************************************************/

/* Add menu commands for a junction
 */
{
    bool is_new = (Junction->m_Flags & IS_NEW) ? TRUE : FALSE;

    if( !is_new )
    {
        if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen()->EEDrawList,
                        WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ), break_line_xpm );
    }

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "delete junction" ), delete_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen()->EEDrawList,
                    WIREITEM | BUSITEM ) )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete node" ), delete_node_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION, _(
                          "Delete connection" ), delete_connection_xpm );
    }
}


/*****************************************************************/
void AddMenusForWire( wxMenu* PopMenu, EDA_DrawLineStruct* Wire,
                      WinEDA_SchematicFrame* frame )
/*****************************************************************/

/* Add menu commands for a wire
 */
{
    bool    is_new = (Wire->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint pos    = frame->GetScreen()->m_Curseur;

    if( is_new )
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "End Wire" ), apply_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Wire" ), delete_xpm );

    if( is_new )
        return;

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete node" ), delete_node_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION, _(
                      "Delete connection" ), delete_connection_xpm );

    if( PickStruct( frame->GetScreen()->m_Curseur, frame->GetScreen()->EEDrawList,
                    WIREITEM | BUSITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ), break_line_xpm );

    PopMenu->AppendSeparator();

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, _( "Add junction" ), add_junction_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, _( "Add label" ), add_line_label_xpm );

    // Place Global label command only if the cursor is over one end of the Wire:
    if( ( pos.x == Wire->m_Start.x && pos.y == Wire->m_Start.y)
       || ( pos.x == Wire->m_End.x && pos.y == Wire->m_End.y ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add global label" ), add_glabel_xpm );
}


/*****************************************************************/
void AddMenusForBus( wxMenu* PopMenu, EDA_DrawLineStruct* Bus,
                     WinEDA_SchematicFrame* frame )
/*****************************************************************/

/* Add menu commands for a Bus
 */
{
    bool    is_new = (Bus->m_Flags & IS_NEW) ? TRUE : FALSE;
    wxPoint pos    = frame->GetScreen()->m_Curseur;

    if( is_new )
        ADD_MENUITEM( PopMenu, ID_POPUP_END_LINE, _( "End Bus" ), apply_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE,
                  _( "Delete Bus" ), delete_bus_xpm );

    if( !is_new )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_BREAK_WIRE,
                      _( "Break Bus" ), break_bus_xpm );

    PopMenu->AppendSeparator();
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, _( "Add junction" ), add_junction_xpm );
    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_LABEL, _( "Add label" ), add_line_label_xpm );

    // Place Global label command only if the cursor is over one end of the Bus:
    if( ( pos.x == Bus->m_Start.x && pos.y == Bus->m_Start.y)
       || ( pos.x == Bus->m_End.x && pos.y == Bus->m_End.y ) )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add global label" ), add_glabel_xpm );
}


/************************************************************************/
void AddMenusForHierchicalSheet( wxMenu* PopMenu, DrawSheetStruct* Sheet )
/************************************************************************/

/* Add menu commands for a Sheet
 */
{
    if( !Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_ENTER_SHEET, _( "Enter Sheet" ), enter_sheet_xpm );
        PopMenu->AppendSeparator();
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_ITEM_REQUEST, _( "Move Sheet" ), move_sheet_xpm );
    }

    if( Sheet->m_Flags )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_END_SHEET, _( "Place Sheet" ), apply_xpm );
    }
    else
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_SHEET, _( "Edit Sheet" ), edit_sheet_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_RESIZE_SHEET, _( "Resize Sheet" ), resize_sheet_xpm );
        if( Sheet->m_Label )  // Sheet has pin labels, and can be cleaned
            ADD_MENUITEM( PopMenu, ID_POPUP_SCH_CLEANUP_SHEET,
                          _( "Cleanup PinSheets" ), options_pinsheet_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete Sheet" ), delete_sheet_xpm );
    }
}


/************************************************************************/
void AddMenusForPinSheet( wxMenu* PopMenu, DrawSheetLabelStruct* PinSheet )
/************************************************************************/

/* Add menu commands for a Pin Sheet (or Sheet label)
 */
{
    if( !PinSheet->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_MOVE_PINSHEET, _( "Move PinSheet" ), move_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_SCH_EDIT_PINSHEET, _( "Edit PinSheet" ), edit_xpm );

    if( !PinSheet->m_Flags )
        ADD_MENUITEM( PopMenu, ID_POPUP_SCH_DELETE, _( "Delete PinSheet" ), delete_pinsheet_xpm );
}


/**********************************************************************/
void AddMenusForBlock( wxMenu* PopMenu, WinEDA_SchematicFrame* frame )
/**********************************************************************/

/* Add menu commands for block
 */
{
    ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel Block" ), cancel_xpm );

    PopMenu->AppendSeparator();

    if( frame->GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
        ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK,
                      _( "Win. Zoom (Midd butt drag mouse)" ), zoom_selected_xpm );

    ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), apply_xpm );

    if( frame->GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
    {
        wxMenu* menu_other_block_commands = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, menu_other_block_commands,
                                   -1, _( "Other block commands" ), right_xpm );
        ADD_MENUITEM( menu_other_block_commands, wxID_COPY, _( "Save Block" ), copy_button );
        ADD_MENUITEM( menu_other_block_commands, ID_POPUP_COPY_BLOCK,
                      _( "Copy Block (shift + drag mouse)" ), copyblock_xpm );
        ADD_MENUITEM( menu_other_block_commands, ID_POPUP_DRAG_BLOCK,
                      _( "Drag Block (ctrl + drag mouse)" ), move_xpm );
        ADD_MENUITEM( menu_other_block_commands, ID_POPUP_DELETE_BLOCK,
                      _( "Del. Block (shift+ctrl + drag mouse)" ), delete_xpm );
        ADD_MENUITEM( menu_other_block_commands, ID_POPUP_MIRROR_Y_BLOCK, _(
                          "Mirror Block ||" ), mirror_H_xpm );
#if 0
  #ifdef __WINDOWS__
        ADD_MENUITEM( menu_other_block_commands, ID_GEN_COPY_BLOCK_TO_CLIPBOARD,
                      _( "Copy to Clipboard" ), copy_button );
  #endif
#endif
    }
}
