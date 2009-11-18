/************************/
/* modedit_onclick.cpp  */
/************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "3d_viewer.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "dialog_edit_module_for_Modedit.h"

#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"


/* Handle the left click in footprint editor
 */
void WinEDA_ModuleEditFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* DrawStruct = GetCurItem();

    DrawPanel->CursorOff( DC );
    if( m_ID_current_state == 0 )
    {
        if( DrawStruct && DrawStruct->m_Flags ) // Command in progress
        {
            switch( DrawStruct->Type() )
            {
            case TYPE_TEXTE_MODULE:
                PlaceTexteModule( (TEXTE_MODULE*) DrawStruct, DC );
                break;

            case TYPE_EDGE_MODULE:
                SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
                Place_EdgeMod( (EDGE_MODULE*) DrawStruct, DC );
                break;

            case TYPE_PAD:
                PlacePad( (D_PAD*) DrawStruct, DC );
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "WinEDA_ModEditFrame::OnLeftClick err: \
m_Flags != 0\nStruct @%p, type %d m_Flag %X" ),
                            DrawStruct, DrawStruct->Type(),
                            DrawStruct->m_Flags );
                DisplayError( this, msg );
                DrawStruct->m_Flags = 0;
                break;
            }
            }
        }
    }

    DrawStruct = GetCurItem();
    if( !DrawStruct || (DrawStruct->m_Flags == 0) )
    {
        if( !wxGetKeyState( WXK_SHIFT ) && !wxGetKeyState( WXK_ALT )
           && !wxGetKeyState( WXK_CONTROL ) )
            DrawStruct = ModeditLocateAndDisplay();
        SetCurItem( DrawStruct );
    }

    switch( m_ID_current_state )
    {
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_ADD_LINE_BUTT:
        if( !DrawStruct || DrawStruct->m_Flags == 0 )
        {
            int shape = S_SEGMENT;
            if( m_ID_current_state == ID_PCB_CIRCLE_BUTT )
                shape = S_CIRCLE;
            if( m_ID_current_state == ID_PCB_ARC_BUTT )
                shape = S_ARC;

            SetCurItem(
                Begin_Edge_Module( (EDGE_MODULE*) NULL, DC, shape ) );
        }
        else if( (DrawStruct->m_Flags & IS_NEW) )
        {
            if( ( (EDGE_MODULE*) DrawStruct )->m_Shape == S_CIRCLE )
            {
                End_Edge_Module( (EDGE_MODULE*) DrawStruct, DC );
                SetCurItem( NULL );
            }
            else if( ( (EDGE_MODULE*) DrawStruct )->m_Shape == S_ARC )
            {
                End_Edge_Module( (EDGE_MODULE*) DrawStruct, DC );
                SetCurItem( NULL );
            }
            else if( ( (EDGE_MODULE*) DrawStruct )->m_Shape == S_SEGMENT )
            {
                SetCurItem(
                    Begin_Edge_Module( (EDGE_MODULE*) DrawStruct, DC, 0 ) );
            }
            else
                DisplayError( this,
                              wxT( "ProcessCommand error: DrawStruct flags error" ) );
        }
        break;

    case ID_MODEDIT_DELETE_ITEM_BUTT:

        // Item in edit, cannot delete it
        if( DrawStruct && (DrawStruct->m_Flags != 0) )
            break;
        DrawStruct = ModeditLocateAndDisplay();
        if( DrawStruct == NULL || (DrawStruct->m_Flags != 0) )
            break;
        if( DrawStruct->Type() != TYPE_MODULE ) //GetBoard()->m_Modules )
        {
            // Cannot delete the module itself
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
            RemoveStruct( DrawStruct );
            DrawStruct = NULL;
            SetCurItem( NULL );
        }
        break;

    case ID_MODEDIT_PLACE_ANCHOR:
    {
        MODULE* module = GetBoard()->m_Modules;
        module->m_Flags = 0;
        SaveCopyInUndoList( module, UR_MODEDIT );
        Place_Ancre( module );      // set the new relatives internal
                                    // coordinates of items
        GetScreen()->m_Curseur = wxPoint( 0, 0 );
        Recadre_Trace( TRUE );

        // Replace the module in position 0, to recalculate absolutes
        // coordinates of items
        module->SetPosition( wxPoint( 0, 0 ) );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        SetCurItem( NULL );
        DrawPanel->Refresh();
    }
    break;

    case ID_PCB_ADD_TEXT_BUTT:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        CreateTextModule( GetBoard()->m_Modules, DC );
        break;

    case ID_MODEDIT_ADD_PAD:
        if( GetBoard()->m_Modules )
        {
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
            AddPad( GetBoard()->m_Modules, true );
        }
        break;

    default:
        DrawPanel->SetCursor( wxCURSOR_ARROW );
        DisplayError( this,
                      wxT( "WinEDA_ModuleEditFrame::ProcessCommand error" ) );
        m_ID_current_state = 0;
        break;
    }

    DrawPanel->CursorOn( DC );
}


/* Handle the right click in the footprint editor:
 * Create the pull up menu
 * After this menu is built, the standard ZOOM menu is added
 */
bool WinEDA_ModuleEditFrame::OnRightClick( const wxPoint& MousePos,
                                           wxMenu*        PopMenu )
{
    BOARD_ITEM* DrawStruct = GetCurItem();
    wxString    msg;
    bool        append_set_width = FALSE;
    bool        BlockActive =
        ( GetScreen()->m_BlockLocate.m_Command !=  BLOCK_IDLE );

    // Simple location of elements where possible.
    if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
    {
        SetCurItem( DrawStruct = ModeditLocateAndDisplay() );
    }

    // End command in progress.
    if(  m_ID_current_state )
    {
        if( DrawStruct && DrawStruct->m_Flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                          _( "Cancel" ), cancel_xpm );
        }
        else
            ADD_MENUITEM( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                          _( "End Tool" ), cancel_tool_xpm );
        PopMenu->AppendSeparator();
    }
    else
    {
        if( (DrawStruct && DrawStruct->m_Flags) || BlockActive )
        {
            if( BlockActive )  // Put block commands in list
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                              _( "Cancel Block" ), cancel_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK,
                              _( "Zoom Block (drag middle mouse)" ),
                              zoom_selected_xpm );
                PopMenu->AppendSeparator();
                ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK,
                              _( "Place Block" ), apply_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_COPY_BLOCK,
                              _( "Copy Block (shift + drag mouse)" ),
                              copyblock_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_X_BLOCK,
                              _( "Mirror Block (alt + drag mouse)" ),
                              mirror_H_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_ROTATE_BLOCK,
                              _( "Rotate Block (ctrl + drag mouse)" ),
                              rotate_pos_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_DELETE_BLOCK,
                              _( "Delete Block (shift+ctrl + drag mouse)" ),
                              delete_xpm );
            }
            else
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                              _( "Cancel" ), cancel_xpm );
            }
            PopMenu->AppendSeparator();
        }
    }

    if( DrawStruct == NULL )
        return true;

    int flags = DrawStruct->m_Flags;

    switch( DrawStruct->Type() )
    {
    case TYPE_MODULE:
    {
        wxMenu* transform_choice = new wxMenu;
        ADD_MENUITEM( transform_choice, ID_MODEDIT_MODULE_ROTATE,
                      _( "Rotate" ), rotate_module_pos_xpm );
        ADD_MENUITEM( transform_choice, ID_MODEDIT_MODULE_MIRROR,
                      _( "Mirror" ), mirror_H_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_MODULE,
                      _( "Edit Module" ), edit_module_xpm );
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, transform_choice,
                                   ID_MODEDIT_TRANSFORM_MODULE,
                                   _( "Transform Module" ), edit_xpm );
        break;
    }

    case TYPE_PAD:
        if( !flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_PAD_REQUEST,
                          _( "Move Pad" ), move_pad_xpm );
        }
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_PAD, _( "Edit Pad" ),
                      options_pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
                      _( "New Pad Settings" ), options_new_pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
                      _( "Export Pad Settings" ), export_options_pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DELETE_PAD,
                      _( "delete Pad" ), delete_pad_xpm );
        if( !flags )
        {
            PopMenu->AppendSeparator();
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                          _( "Global Pad Settings" ), global_options_pad_xpm );
        }
        break;

    case TYPE_TEXTE_MODULE:
        if( !flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
                          _( "Move Text Mod." ), move_field_xpm );
        }
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_ROTATE_TEXTMODULE,
                      _( "Rotate Text Mod." ), rotate_field_xpm );
        if( !flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_TEXTMODULE,
                          _( "Edit Text Mod." ), edit_text_xpm );
            if( ( (TEXTE_MODULE*) DrawStruct )->m_Type == TEXT_is_DIVERS )
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DELETE_TEXTMODULE,
                              _( "Delete Text Mod." ), delete_text_xpm );
        }
        break;

    case TYPE_EDGE_MODULE:
    {
        if( (flags & IS_NEW) )
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING,
                          _( "End edge" ), apply_xpm );
        if( !flags )
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_EDGE,
                          _( "Move edge" ), move_line_xpm );
        if( ( flags & (IS_NEW | IS_MOVED) ) == IS_MOVED )
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_PLACE_EDGE,
                          _( "Place edge" ), apply_xpm );
        wxMenu* edit_mnu = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, edit_mnu,
                                   ID_POPUP_PCB_EDIT_EDGE, _(
                                       "Edit" ), edit_xpm );
        ADD_MENUITEM( edit_mnu, ID_POPUP_PCB_EDIT_WIDTH_CURRENT_EDGE,
                      _( "Edit Width (Current)" ), width_segment_xpm );
        ADD_MENUITEM( edit_mnu, ID_POPUP_PCB_EDIT_WIDTH_ALL_EDGE,
                      _( "Edit Width (All)" ), width_segment_xpm );
        ADD_MENUITEM( edit_mnu, ID_POPUP_PCB_EDIT_LAYER_CURRENT_EDGE,
                      _( "Edit Layer (Current)" ), select_layer_pair_xpm );
        ADD_MENUITEM( edit_mnu, ID_POPUP_PCB_EDIT_LAYER_ALL_EDGE,
                      _( "Edit Layer (All)" ), select_layer_pair_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DELETE_EDGE,
                      _( "Delete edge" ), delete_xpm );
        append_set_width = TRUE;
    }
    break;

    case TYPE_DRAWSEGMENT:
    case TYPE_TEXTE:
    case TYPE_VIA:
    case TYPE_TRACK:
    case TYPE_ZONE:
    case TYPE_MARKER_PCB:
    case TYPE_COTATION:
    case TYPE_MIRE:
        break;

    case TYPE_SCREEN:
    case TYPE_NOT_INIT:
    case TYPE_PCB:
        msg.Printf( wxT( "WinEDA_ModuleEditFrame::OnRightClick Error: illegal DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;

    default:
        msg.Printf( wxT( "WinEDA_ModuleEditFrame::OnRightClick Error: unknown DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();

    if( append_set_width
       || ( m_ID_current_state
           && ( ( m_ID_current_state == ID_PCB_ADD_LINE_BUTT )
               || ( m_ID_current_state == ID_PCB_CIRCLE_BUTT )
               || ( m_ID_current_state == ID_PCB_ARC_BUTT ) ) ) )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_ENTER_EDGE_WIDTH,
                      _( "Set Width" ), width_segment_xpm );
        PopMenu->AppendSeparator();
    }

    return true;
}


/*  Handle the double click in the footprint editor:
 *  If the double clicked item is editable: call the corresponding editor.
 */
void WinEDA_ModuleEditFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* DrawStruct = GetCurItem();
    wxPoint     pos = GetPosition();
    wxClientDC  dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( m_ID_current_state )
    {
    case 0:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            DrawStruct = ModeditLocateAndDisplay();
        }

        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags != 0 ) )
            break;

        // Item found
        SetCurItem( DrawStruct );

        switch( DrawStruct->Type() )
        {
        case TYPE_PAD:
            InstallPadOptionsFrame( (D_PAD*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_MODULE:
        {
            DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) DrawStruct );
            int ret = dialog.ShowModal();
            GetScreen()->GetCurItem()->m_Flags = 0;
            GetScreen()->GetCurItem()->m_Flags = 0;
            DrawPanel->MouseToCursorSchema();
            if( ret > 0 )
                DrawPanel->Refresh();
        }
        break;

        case TYPE_TEXTE_MODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) DrawStruct, &dc );
            DrawPanel->MouseToCursorSchema();
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_PCB_ADD_LINE_BUTT:
    {
        if( DrawStruct && ( DrawStruct->m_Flags & IS_NEW ) )
        {
            End_Edge_Module( (EDGE_MODULE*) DrawStruct, DC );
            SetCurItem( NULL );
        }
        break;
    }

    default:
        break;
    }
}
