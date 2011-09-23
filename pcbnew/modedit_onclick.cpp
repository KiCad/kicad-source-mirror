/**
 * @file modedit_onclick.cpp
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "3d_viewer.h"
#include "wxPcbStruct.h"
#include "gr_basic.h"

#include "class_board.h"
#include "class_module.h"
#include "class_edge_mod.h"

#include "pcbnew.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "module_editor_frame.h"
#include "dialog_edit_module_for_Modedit.h"


void FOOTPRINT_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* item = GetCurItem();

    DrawPanel->CrossHairOff( DC );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        if( item && item->m_Flags ) // Move item command in progress
        {
            switch( item->Type() )
            {
            case TYPE_TEXTE_MODULE:
                PlaceTexteModule( (TEXTE_MODULE*) item, DC );
                break;

            case TYPE_EDGE_MODULE:
                SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
                Place_EdgeMod( (EDGE_MODULE*) item );
                break;

            case TYPE_PAD:
                PlacePad( (D_PAD*) item, DC );
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "WinEDA_ModEditFrame::OnLeftClick err:Struct %d, m_Flag %X" ),
                            item->Type(), item->m_Flags );
                DisplayError( this, msg );
                item->m_Flags = 0;
                break;
            }
            }
        }
    }

    item = GetCurItem();

    if( !item || (item->m_Flags == 0) )
    {
        if( !wxGetKeyState( WXK_SHIFT ) && !wxGetKeyState( WXK_ALT )
           && !wxGetKeyState( WXK_CONTROL ) )
            item = ModeditLocateAndDisplay();

        SetCurItem( item );
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_MODEDIT_CIRCLE_TOOL:
    case ID_MODEDIT_ARC_TOOL:
    case ID_MODEDIT_LINE_TOOL:
        if( !item || item->m_Flags == 0 )
        {
            int shape = S_SEGMENT;

            if( GetToolId() == ID_MODEDIT_CIRCLE_TOOL )
                shape = S_CIRCLE;

            if( GetToolId() == ID_MODEDIT_ARC_TOOL )
                shape = S_ARC;

            SetCurItem( Begin_Edge_Module( (EDGE_MODULE*) NULL, DC, shape ) );
        }
        else if( item->IsNew() )
        {
            if( ( (EDGE_MODULE*) item )->m_Shape == S_CIRCLE )
            {
                End_Edge_Module( (EDGE_MODULE*) item );
                SetCurItem( NULL );
                DrawPanel->Refresh();
            }
            else if( ( (EDGE_MODULE*) item )->m_Shape == S_ARC )
            {
                End_Edge_Module( (EDGE_MODULE*) item );
                SetCurItem( NULL );
                DrawPanel->Refresh();
            }
            else if( ( (EDGE_MODULE*) item )->m_Shape == S_SEGMENT )
            {
                SetCurItem( Begin_Edge_Module( (EDGE_MODULE*) item, DC, 0 ) );
            }
            else
            {
                DisplayError( this, wxT( "ProcessCommand error: item flags error" ) );
            }
        }
        break;

    case ID_MODEDIT_DELETE_TOOL:
        if( item == NULL ||           // No item to delete
            (item->m_Flags != 0) )    // Item in edit, cannot delete it
            break;

        if( item->Type() != TYPE_MODULE ) // Cannot delete the module itself
        {
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
            RemoveStruct( item );
            SetCurItem( NULL );
        }

        break;

    case ID_MODEDIT_ANCHOR_TOOL:
    {
        MODULE* module = GetBoard()->m_Modules;

        if( module == NULL    // No module loaded
            || (module->m_Flags != 0) )
            break;

        module->m_Flags = 0;
        SaveCopyInUndoList( module, UR_MODEDIT );
        Place_Ancre( module );      // set the new relatives internal coordinates of items
        RedrawScreen( wxPoint( 0, 0 ), true );

        // Replace the module in position 0, to recalculate absolutes coordinates of items
        module->SetPosition( wxPoint( 0, 0 ) );
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        SetCurItem( NULL );
        DrawPanel->Refresh();
    }
    break;

    case ID_MODEDIT_PLACE_GRID_COORD:
        DrawPanel->DrawGridAxis( DC, GR_XOR );
        GetScreen()->m_GridOrigin = GetScreen()->GetCrossHairPosition();
        DrawPanel->DrawGridAxis( DC, GR_COPY );
        GetScreen()->SetModify();
        break;

    case ID_MODEDIT_TEXT_TOOL:
        if( GetBoard()->m_Modules == NULL )
            break;

        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        CreateTextModule( GetBoard()->m_Modules, DC );
        break;

    case ID_MODEDIT_PAD_TOOL:
        if( GetBoard()->m_Modules )
        {
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
            AddPad( GetBoard()->m_Modules, true );
        }

        break;

    default:
        DisplayError( this, wxT( "FOOTPRINT_EDIT_FRAME::ProcessCommand error" ) );
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
    }

    DrawPanel->CrossHairOn( DC );
}


bool FOOTPRINT_EDIT_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    BOARD_ITEM* item = GetCurItem();
    wxString    msg;
    bool        append_set_width = false;
    bool        blockActive = GetScreen()->m_BlockLocate.m_Command !=  BLOCK_IDLE;

    // Simple location of elements where possible.
    if( ( item == NULL ) || ( item->m_Flags == 0 ) )
    {
        SetCurItem( item = ModeditLocateAndDisplay() );
    }

    // End command in progress.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( item && item->m_Flags )
            AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), KiBitmap( cancel_xpm ) );
        else
            AddMenuItem( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL, _( "End Tool" ), KiBitmap( cancel_tool_xpm ) );

        PopMenu->AppendSeparator();
    }
    else
    {
        if( (item && item->m_Flags) || blockActive )
        {
            if( blockActive )  // Put block commands in list
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel Block" ), KiBitmap( cancel_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_ZOOM_BLOCK,
                             _( "Zoom Block (drag middle mouse)" ),
                             KiBitmap( zoom_area_xpm ) );
                PopMenu->AppendSeparator();
                AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK,
                             _( "Place Block" ), KiBitmap( apply_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_COPY_BLOCK,
                             _( "Copy Block (shift + drag mouse)" ),
                             KiBitmap( copyblock_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_MIRROR_X_BLOCK,
                             _( "Mirror Block (alt + drag mouse)" ),
                             KiBitmap( mirror_h_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_ROTATE_BLOCK,
                             _( "Rotate Block (ctrl + drag mouse)" ),
                             KiBitmap( rotate_ccw_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK,
                             _( "Delete Block (shift+ctrl + drag mouse)" ),
                             KiBitmap( delete_xpm ) );
            }
            else
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), KiBitmap( cancel_xpm ) );
            }

            PopMenu->AppendSeparator();
        }
    }

    if( (item == NULL) || blockActive )
        return true;

    int flags = item->m_Flags;

    switch( item->Type() )
    {
    case TYPE_MODULE:
    {
        wxMenu* transform_choice = new wxMenu;
        AddMenuItem( transform_choice, ID_MODEDIT_MODULE_ROTATE, _( "Rotate" ),
                     KiBitmap( rotate_module_pos_xpm ) );
        AddMenuItem( transform_choice, ID_MODEDIT_MODULE_MIRROR, _( "Mirror" ), KiBitmap( mirror_h_xpm ) );
        msg = AddHotkeyName( _( "Edit Module" ), g_Module_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_MODULE, msg, KiBitmap( edit_module_xpm ) );
        AddMenuItem( PopMenu, transform_choice, ID_MODEDIT_TRANSFORM_MODULE,
                     _( "Transform Module" ), KiBitmap( edit_xpm ) );
        break;
    }

    case TYPE_PAD:
        if( !flags )
        {
            msg = AddHotkeyName( _("Move Pad" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_PAD_REQUEST, msg, KiBitmap( move_pad_xpm ) );
        }

        msg = AddHotkeyName( _("Edit Pad" ), g_Module_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_PAD, msg, KiBitmap( options_pad_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
                     _( "New Pad Settings" ), KiBitmap( options_new_pad_xpm ) );
        AddMenuItem( PopMenu, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
                     _( "Export Pad Settings" ), KiBitmap( export_options_pad_xpm ) );
        msg = AddHotkeyName( _("Delete Pad" ), g_Module_Editor_Hokeys_Descr, HK_DELETE );
        AddMenuItem( PopMenu, ID_POPUP_PCB_DELETE_PAD, msg, KiBitmap( delete_pad_xpm ) );

        if( !flags )
        {
            PopMenu->AppendSeparator();
            AddMenuItem( PopMenu, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                         _( "Global Pad Settings" ), KiBitmap( global_options_pad_xpm ) );
        }

        break;

    case TYPE_TEXTE_MODULE:
        if( !flags )
        {
            msg = AddHotkeyName( _("Move Text Mod." ), g_Module_Editor_Hokeys_Descr,
                                 HK_MOVE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST, msg, KiBitmap( move_field_xpm ) );
        }

        msg = AddHotkeyName( _("Rotate Text Mod." ), g_Module_Editor_Hokeys_Descr,
                             HK_ROTATE_ITEM );
        AddMenuItem( PopMenu, ID_POPUP_PCB_ROTATE_TEXTMODULE, msg, KiBitmap( rotate_field_xpm ) );

        if( !flags )
        {
            msg = AddHotkeyName( _("Edit Text Mod." ), g_Module_Editor_Hokeys_Descr,
                                 HK_EDIT_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_TEXTMODULE, msg, KiBitmap( edit_text_xpm ) );

            if( ( (TEXTE_MODULE*) item )->m_Type == TEXT_is_DIVERS )
            {
                msg = AddHotkeyName( _("Delete Text Mod." ), g_Module_Editor_Hokeys_Descr,
                                     HK_DELETE );
                AddMenuItem( PopMenu, ID_POPUP_PCB_DELETE_TEXTMODULE, msg, KiBitmap( delete_text_xpm ) );
            }
        }
        break;

    case TYPE_EDGE_MODULE:
    {
        if( (flags & IS_NEW) )
            AddMenuItem( PopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING, _( "End edge" ), KiBitmap( apply_xpm ) );

        if( !flags )
        {
            msg = AddHotkeyName( _("Move edge" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EDGE, msg, KiBitmap( move_line_xpm ) );
        }

        if( ( flags & (IS_NEW | IS_MOVED) ) == IS_MOVED )
            AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_EDGE, _( "Place edge" ), KiBitmap( apply_xpm ) );

        wxMenu* edit_mnu = new wxMenu;
        AddMenuItem( PopMenu, edit_mnu, ID_POPUP_PCB_EDIT_EDGE, _( "Edit" ), KiBitmap( edit_xpm ) );
        AddMenuItem( edit_mnu, ID_POPUP_PCB_EDIT_WIDTH_CURRENT_EDGE,
                     _( "Edit Width (Current)" ), KiBitmap( width_segment_xpm ) );
        AddMenuItem( edit_mnu, ID_POPUP_PCB_EDIT_WIDTH_ALL_EDGE,
                     _( "Edit Width (All)" ), KiBitmap( width_segment_xpm ) );
        AddMenuItem( edit_mnu, ID_POPUP_PCB_EDIT_LAYER_CURRENT_EDGE,
                     _( "Edit Layer (Current)" ), KiBitmap( select_layer_pair_xpm ) );
        AddMenuItem( edit_mnu, ID_POPUP_PCB_EDIT_LAYER_ALL_EDGE,
                     _( "Edit Layer (All)" ), KiBitmap( select_layer_pair_xpm ) );
        msg = AddHotkeyName( _("Delete edge" ), g_Module_Editor_Hokeys_Descr, HK_DELETE );

        AddMenuItem( PopMenu, ID_POPUP_PCB_DELETE_EDGE, msg, KiBitmap( delete_xpm ) );
        append_set_width = true;
    }
    break;

    case TYPE_DRAWSEGMENT:
    case TYPE_TEXTE:
    case TYPE_VIA:
    case TYPE_TRACK:
    case TYPE_ZONE:
    case TYPE_MARKER_PCB:
    case TYPE_DIMENSION:
    case PCB_TARGET_T:
        break;

    case TYPE_SCREEN:
    case TYPE_NOT_INIT:
    case TYPE_PCB:
        msg.Printf( wxT( "FOOTPRINT_EDIT_FRAME::OnRightClick Error: illegal DrawType %d" ),
                    item->Type() );
        DisplayError( this, msg );
        break;

    default:
        msg.Printf( wxT( "FOOTPRINT_EDIT_FRAME::OnRightClick Error: unknown DrawType %d" ),
                    item->Type() );
        DisplayError( this, msg );
        break;
    }

    PopMenu->AppendSeparator();

    if( append_set_width
        || ( ( GetToolId() != ID_NO_TOOL_SELECTED )
           && ( ( GetToolId() == ID_PCB_ADD_LINE_BUTT )
               || ( GetToolId() == ID_PCB_CIRCLE_BUTT )
               || ( GetToolId() == ID_PCB_ARC_BUTT ) ) ) )
    {
        AddMenuItem( PopMenu, ID_POPUP_PCB_ENTER_EDGE_WIDTH, _("Set Width" ), KiBitmap( width_segment_xpm ) );
        PopMenu->AppendSeparator();
    }

    return true;
}


void FOOTPRINT_EDIT_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* item = GetCurItem();

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( ( item == NULL ) || ( item->m_Flags == 0 ) )
        {
            item = ModeditLocateAndDisplay();
        }

        if( ( item == NULL ) || ( item->m_Flags != 0 ) )
            break;

        // Item found
        SetCurItem( item );

        switch( item->Type() )
        {
        case TYPE_PAD:
            InstallPadOptionsFrame( (D_PAD*) item );
            DrawPanel->MoveCursorToCrossHair();
            break;

        case TYPE_MODULE:
        {
            DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) item );
            int ret = dialog.ShowModal();
            GetScreen()->GetCurItem()->m_Flags = 0;
            DrawPanel->MoveCursorToCrossHair();

            if( ret > 0 )
                DrawPanel->Refresh();
        }
        break;

        case TYPE_TEXTE_MODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) item, DC );
            DrawPanel->MoveCursorToCrossHair();
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_PCB_ADD_LINE_BUTT:
    {
        if( item && item->IsNew() )
        {
            End_Edge_Module( (EDGE_MODULE*) item );
            SetCurItem( NULL );
            DrawPanel->Refresh();
        }

        break;
    }

    default:
        break;
    }
}
