/************************/
/* modedit_onclick.cpp  */
/************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"

/*************************************************************************/
void WinEDA_ModuleEditFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/*************************************************************************/

/* Handle the left click in footprint editor
 */
{
    BOARD_ITEM* DrawStruct = GetCurItem();

    DrawPanel->CursorOff( DC );
    if( m_ID_current_state == 0 )
    {
        if( DrawStruct && DrawStruct->m_Flags ) // Command in progress
        {
            switch( DrawStruct->Type() )
            {
            case TYPETEXTEMODULE:
                SaveCopyInUndoList( m_Pcb->m_Modules );
                PlaceTexteModule( (TEXTE_MODULE*) DrawStruct, DC );
                break;

            case TYPEEDGEMODULE:
                SaveCopyInUndoList( m_Pcb->m_Modules );
                Place_EdgeMod( (EDGE_MODULE*) DrawStruct, DC );
                break;

            case TYPEPAD:
                PlacePad( (D_PAD*) DrawStruct, DC );
                break;

            default:
            {
                wxString msg;
                msg.Printf(
                    wxT(
                        "WinEDA_ModEditFrame::ProcessCommand err: m_Flags != 0\nStruct @%p, type %d m_Flag %X" ),
                    DrawStruct, DrawStruct->Type(), DrawStruct->m_Flags );
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
        if( !wxGetKeyState(WXK_SHIFT) && !wxGetKeyState(WXK_ALT) &&
                !wxGetKeyState(WXK_CONTROL) )
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
    case ID_LINE_COMMENT_BUTT:
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
                DisplayError( this, wxT( "ProcessCommand error: DrawStruct/ flags error" ) );
        }
        break;

    case ID_MODEDIT_DELETE_ITEM_BUTT:
        if( !DrawStruct || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = ModeditLocateAndDisplay();
            if( DrawStruct && (DrawStruct->m_Flags == 0) )
            {
                SaveCopyInUndoList( m_Pcb->m_Modules );
                RemoveStruct( DrawStruct, DC );
                SetCurItem( DrawStruct = NULL );
            }
        }
        break;

    case ID_MODEDIT_PLACE_ANCHOR:
        SaveCopyInUndoList( m_Pcb->m_Modules );
        Place_Ancre( m_Pcb->m_Modules, DC );
        m_Pcb->m_Modules->m_Flags  = 0;
        GetScreen()->m_Curseur = wxPoint( 0, 0 );
        Recadre_Trace( TRUE );
        Place_Module( m_Pcb->m_Modules, DC );
        RedrawActiveWindow( DC, TRUE );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        SetCurItem( NULL );
        break;

    case ID_TEXT_COMMENT_BUTT:
        SaveCopyInUndoList( m_Pcb->m_Modules );
        CreateTextModule( m_Pcb->m_Modules, DC );
        break;

    case ID_MODEDIT_ADD_PAD:
        if( m_Pcb->m_Modules )
        {
            SaveCopyInUndoList( m_Pcb->m_Modules );
            AddPad( m_Pcb->m_Modules, true );
        }
        break;

    default:
        DrawPanel->SetCursor( wxCURSOR_ARROW );
        DisplayError( this, wxT( "WinEDA_ModuleEditFrame::ProcessCommand error" ) );
        m_ID_current_state = 0;
        break;
    }

    DrawPanel->CursorOn( DC );
}


/*********************************************************************/
bool WinEDA_ModuleEditFrame::OnRightClick( const wxPoint& MousePos,
                                           wxMenu*        PopMenu )
/*********************************************************************/

/* Handle the right click in the footprint editor:
 * Create the pull up menu
 * After this menu is built, the standart ZOOM menu is added
 */
{
    BOARD_ITEM*     DrawStruct = GetCurItem();
    wxString        msg;
    bool            append_set_width = FALSE;
    bool            BlockActive = (GetScreen()->BlockLocate.m_Command !=  BLOCK_IDLE);

    // Simple localisation des elements si possible
    if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
    {
        SetCurItem( DrawStruct = ModeditLocateAndDisplay() );
    }

    // Si commande en cours: affichage fin de commande
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
            if( BlockActive )  // Put block commnands in list
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                              _( "Cancel Block" ), cancel_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_ZOOM_BLOCK,
                              _( "Zoom Block (drag middle mouse)" ), zoom_selected_xpm );
                PopMenu->AppendSeparator();
                ADD_MENUITEM( PopMenu, ID_POPUP_PLACE_BLOCK,
                              _( "Place Block" ), apply_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_COPY_BLOCK,
                              _( "Copy Block (shift + drag mouse)" ), copyblock_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_MIRROR_X_BLOCK,
                              _( "Mirror Block (alt + drag mouse)" ), mirror_H_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_ROTATE_BLOCK,
                              _( "Rotate Block (ctrl + drag mouse)" ), rotate_pos_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_DELETE_BLOCK,
                              _( "Delete Block (shift+ctrl + drag mouse)" ), delete_xpm );
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
    case TYPEMODULE:
    {
        wxMenu* transform_choice = new wxMenu;
        ADD_MENUITEM( transform_choice, ID_MODEDIT_MODULE_ROTATE,
                      _( "Rotate" ), rotate_module_pos_xpm );
        ADD_MENUITEM( transform_choice, ID_MODEDIT_MODULE_MIRROR,
                      _( "Mirror" ), mirror_H_xpm );
#if 0
        transform_choice->Append( ID_MODEDIT_MODULE_SCALE, _( "Scale" ) );
        transform_choice->Append( ID_MODEDIT_MODULE_SCALE, _( "Scale X" ) );
        transform_choice->Append( ID_MODEDIT_MODULE_SCALE, _( "Scale Y" ) );
#endif
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_MODULE,
                      _( "Edit Module" ), Edit_Module_xpm );
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, transform_choice,
                                   ID_MODEDIT_TRANSFORM_MODULE,
                                   _( "Transform Module" ), edit_xpm );
        break;
    }

    case TYPEPAD:
        if( !flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_PAD_REQUEST,
                          _( "Move Pad" ), move_pad_xpm );
        }
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_PAD, _( "Edit Pad" ), options_pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
                      _( "New Pad Settings" ), options_new_pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
                      _( "Export Pad Settings" ), Export_Options_Pad_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DELETE_PAD,
                      _( "delete Pad" ), Delete_Pad_xpm );
        if( !flags )
        {
            PopMenu->AppendSeparator();
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                          _( "Global Pad Settings" ), global_options_pad_xpm );
        }
        break;

    case TYPETEXTEMODULE:
        if( !flags )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
                          _( "Move Text Mod." ), move_field_xpm );
        }
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_ROTATE_TEXTMODULE,
                      _( "Rotate Text Mod." ), rotate_field_xpm );
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_EDIT_TEXTMODULE,
                      _( "Edit Text Mod." ), edit_text_xpm );
        if( ( (TEXTE_MODULE*) DrawStruct )->m_Type == TEXT_is_DIVERS )
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DELETE_TEXTMODULE,
                          _( "Delete Text Mod." ), delete_text_xpm );
        break;

    case TYPEEDGEMODULE:
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
                                   ID_POPUP_PCB_EDIT_EDGE, _( "Edit" ), edit_xpm );
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

    case TYPEDRAWSEGMENT:
    case TYPETEXTE:
    case TYPEVIA:
    case TYPETRACK:
    case TYPEZONE:
    case TYPEMARKER:
    case TYPECOTATION:
    case TYPEMIRE:
        break;

    case TYPESCREEN:
    case TYPE_NOT_INIT:
    case TYPEPCB:
    case PCB_EQUIPOT_STRUCT_TYPE:
        msg.Printf(
            wxT( "WinEDA_ModuleEditFrame::OnRightClick Error: illegal DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;

    default:
        msg.Printf(
            wxT( "WinEDA_ModuleEditFrame::OnRightClick Error: unknown DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();

    if( append_set_width
       || ( m_ID_current_state && ( (m_ID_current_state == ID_LINE_COMMENT_BUTT)
                                   || (m_ID_current_state == ID_PCB_CIRCLE_BUTT)
                                   || (m_ID_current_state == ID_PCB_ARC_BUTT) ) ) )
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_ENTER_EDGE_WIDTH,
                      _( "Set Width" ), width_segment_xpm );
        PopMenu->AppendSeparator();
    }

    return true;
}


/****************************************************************************/
void WinEDA_ModuleEditFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/****************************************************************************/

/*  Handle the double click in the footprin editor:
 *  If the double clicked item is editable: call the corresponding editor.
 */
{
    BOARD_ITEM*     DrawStruct = GetCurItem();
    wxPoint         pos = GetPosition();
    wxClientDC      dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = ModeditLocateAndDisplay();
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags != 0) )
            break;

        // Item found
        SetCurItem( DrawStruct );

        switch( DrawStruct->Type() )
        {
        case TYPEPAD:
            InstallPadOptionsFrame(
                (D_PAD*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEMODULE:
            InstallModuleOptionsFrame( (MODULE*) DrawStruct,
                                      &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPETEXTEMODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) DrawStruct,
                                       &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_LINE_COMMENT_BUTT:
    {
        if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
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
