/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file modedit_onclick.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <gr_basic.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <module_editor_frame.h>
#include <dialog_edit_module_for_Modedit.h>
#include <menus_helpers.h>


void FOOTPRINT_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* item = GetCurItem();

    m_canvas->CrossHairOff( DC );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        if( item && item->GetFlags() ) // Move item command in progress
        {
            switch( item->Type() )
            {
            case PCB_MODULE_TEXT_T:
                PlaceTexteModule( static_cast<TEXTE_MODULE*>( item ), DC );
                break;

            case PCB_MODULE_EDGE_T:
                SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
                Place_EdgeMod( static_cast<EDGE_MODULE*>( item ) );
                break;

            case PCB_PAD_T:
                PlacePad( static_cast<D_PAD*>( item ), DC );
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "WinEDA_ModEditFrame::OnLeftClick err:Struct %d, m_Flag %X" ),
                            item->Type(), item->GetFlags() );
                DisplayError( this, msg );
                item->ClearFlags();
                break;
            }
            }
        }

        else
        {
            if( !wxGetKeyState( WXK_SHIFT ) && !wxGetKeyState( WXK_ALT )
               && !wxGetKeyState( WXK_CONTROL ) )
                item = ModeditLocateAndDisplay();

            SetCurItem( item );
        }
    }

    item = GetCurItem();
    bool no_item_edited = item == NULL || item->GetFlags() == 0;

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_MODEDIT_CIRCLE_TOOL:
    case ID_MODEDIT_ARC_TOOL:
    case ID_MODEDIT_LINE_TOOL:
        if( no_item_edited )
        {
            STROKE_T shape = S_SEGMENT;

            if( GetToolId() == ID_MODEDIT_CIRCLE_TOOL )
                shape = S_CIRCLE;

            if( GetToolId() == ID_MODEDIT_ARC_TOOL )
                shape = S_ARC;

            SetCurItem( Begin_Edge_Module( (EDGE_MODULE*) NULL, DC, shape ) );
        }
        else if( item->IsNew() )
        {
            if( ( (EDGE_MODULE*) item )->GetShape() == S_CIRCLE )
            {
                End_Edge_Module( (EDGE_MODULE*) item );
                SetCurItem( NULL );
                m_canvas->Refresh();
            }
            else if( ( (EDGE_MODULE*) item )->GetShape() == S_ARC )
            {
                End_Edge_Module( (EDGE_MODULE*) item );
                SetCurItem( NULL );
                m_canvas->Refresh();
            }
            else if( ( (EDGE_MODULE*) item )->GetShape() == S_SEGMENT )
            {
                SetCurItem( Begin_Edge_Module( (EDGE_MODULE*) item, DC, S_SEGMENT ) );
            }
            else
            {
                wxMessageBox( wxT( "ProcessCommand error: unknown shape" ) );
            }
        }
        break;

    case ID_MODEDIT_DELETE_TOOL:
        if( ! no_item_edited )    // Item in edit, cannot delete it
            break;

        item = ModeditLocateAndDisplay();

        if( item && item->Type() != PCB_MODULE_T ) // Cannot delete the module itself
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
                || (module->GetFlags() != 0) )
                break;

            SaveCopyInUndoList( module, UR_MODEDIT );

            // set the new relative internal local coordinates of footprint items
            wxPoint moveVector = module->GetPosition() - GetCrossHairPosition();
            module->MoveAnchorPosition( moveVector );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
            SetCurItem( NULL );
            m_canvas->Refresh();
        }
        break;

    case ID_MODEDIT_PLACE_GRID_COORD:
        m_canvas->DrawGridAxis( DC, GR_XOR, GetBoard()->GetGridOrigin() );
        SetGridOrigin( GetCrossHairPosition() );
        m_canvas->DrawGridAxis( DC, GR_COPY, GetBoard()->GetGridOrigin() );
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
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
    }

    m_canvas->CrossHairOn( DC );
}


bool FOOTPRINT_EDIT_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    BOARD_ITEM* item = GetCurItem();
    wxString    msg;
    bool        blockActive = !GetScreen()->m_BlockLocate.IsIdle();

    // Simple location of elements where possible.
    if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
    {
        SetCurItem( item = ModeditLocateAndDisplay() );
    }

    // End command in progress.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( item && item->GetFlags() )
            AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ),
                         KiBitmap( cancel_xpm ) );
        else
            AddMenuItem( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL, _( "End Tool" ),
                         KiBitmap( cursor_xpm ) );

        PopMenu->AppendSeparator();
    }
    else
    {
        if( (item && item->GetFlags()) || blockActive )
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
                             _( "Place Block" ), KiBitmap( checked_ok_xpm ) );
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

                msg = AddHotkeyName( _("Move Block Exactly" ),
                        g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM_EXACT );
                AddMenuItem( PopMenu, ID_POPUP_MOVE_BLOCK_EXACT,
                             msg, KiBitmap( move_xpm ) );
            }
            else
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), KiBitmap( cancel_xpm ) );
            }

            PopMenu->AppendSeparator();
        }
    }

    if( blockActive )
        return true;

    if( item  )
    {
        STATUS_FLAGS flags = item->GetFlags();
        switch( item->Type() )
        {
        case PCB_MODULE_T:
            {
            wxMenu* transform_choice = new wxMenu;
            AddMenuItem( transform_choice, ID_MODEDIT_MODULE_ROTATE, _( "Rotate" ),
                         KiBitmap( rotate_module_ccw_xpm ) );
            AddMenuItem( transform_choice, ID_MODEDIT_MODULE_MIRROR, _( "Mirror" ),
                         KiBitmap( mirror_footprint_axisY_xpm ) );
            AddMenuItem( transform_choice, ID_MODEDIT_MODULE_MOVE_EXACT, _( "Move Exactly" ),
                         KiBitmap( move_module_xpm ) );

            msg = AddHotkeyName( _( "Edit Footprint" ), g_Module_Editor_Hokeys_Descr, HK_EDIT_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_MODULE_PRMS, msg, KiBitmap( edit_module_xpm ) );
            AddMenuItem( PopMenu, transform_choice, ID_MODEDIT_TRANSFORM_MODULE,
                         _( "Transform Footprint" ), KiBitmap( edit_xpm ) );

            break;
            }

        case PCB_PAD_T:
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

            msg = AddHotkeyName( _( "Duplicate Pad" ), g_Module_Editor_Hokeys_Descr, HK_DUPLICATE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_DUPLICATE_ITEM, msg, KiBitmap( duplicate_pad_xpm ) );

            msg = AddHotkeyName( _("Move Pad Exactly" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM_EXACT );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EXACT, msg, KiBitmap( move_pad_xpm ) );

            msg = AddHotkeyName( _("Create Pad Array" ), g_Module_Editor_Hokeys_Descr, HK_CREATE_ARRAY );
            AddMenuItem( PopMenu, ID_POPUP_PCB_CREATE_ARRAY, msg, KiBitmap( array_pad_xpm ) );


            if( !flags )
            {
                PopMenu->AppendSeparator();
                AddMenuItem( PopMenu, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                             _( "Global Pad Settings" ), KiBitmap( global_options_pad_xpm ) );
            }

            break;

        case PCB_MODULE_TEXT_T:
            if( !flags )
            {
                msg = AddHotkeyName( _("Move Text" ), g_Module_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST, msg,
                             KiBitmap( move_field_xpm ) );
            }

            msg = AddHotkeyName( _("Rotate Text" ), g_Module_Editor_Hokeys_Descr,
                                 HK_ROTATE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_ROTATE_TEXTMODULE, msg, KiBitmap( rotate_field_xpm ) );

            {
                // Do not show option to replicate value or reference fields
                // (there can only be one of each)

                const MODULE* module = static_cast<MODULE*>( item->GetParent() );
                const TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

                if( &module->Reference() != text && &module->Value() != text )
                {
                    msg = AddHotkeyName( _( "Duplicate Text" ),
                                         g_Module_Editor_Hokeys_Descr, HK_DUPLICATE_ITEM );
                    AddMenuItem( PopMenu, ID_POPUP_PCB_DUPLICATE_ITEM,
                                 msg, KiBitmap( duplicate_text_xpm ) );

                    msg = AddHotkeyName( _("Create Text Array" ),
                                         g_Module_Editor_Hokeys_Descr, HK_CREATE_ARRAY );
                    AddMenuItem( PopMenu, ID_POPUP_PCB_CREATE_ARRAY,
                                 msg, KiBitmap( array_text_xpm ) );
                }
            }

            msg = AddHotkeyName( _("Move Text Exactly" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM_EXACT );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EXACT, msg, KiBitmap( move_field_xpm ) );

            if( !flags )
            {
                msg = AddHotkeyName( _("Edit Text" ), g_Module_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_TEXTMODULE, msg, KiBitmap( edit_text_xpm ) );

                if( ( static_cast<TEXTE_MODULE*>( item ) )->GetType() == TEXTE_MODULE::TEXT_is_DIVERS )
                {
                    msg = AddHotkeyName( _("Delete Text" ), g_Module_Editor_Hokeys_Descr,
                                         HK_DELETE );
                    AddMenuItem( PopMenu, ID_POPUP_PCB_DELETE_TEXTMODULE, msg,
                                 KiBitmap( delete_text_xpm ) );
                }
            }
            break;

        case PCB_MODULE_EDGE_T:
        {
            if( (flags & IS_NEW) )
                AddMenuItem( PopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING, _( "End edge" ),
                             KiBitmap( checked_ok_xpm ) );

            if( !flags )
            {
                msg = AddHotkeyName( _("Move Edge" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EDGE, msg, KiBitmap( move_line_xpm ) );
            }

            msg = AddHotkeyName( _( "Duplicate Edge" ), g_Module_Editor_Hokeys_Descr, HK_DUPLICATE_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_DUPLICATE_ITEM, msg, KiBitmap( duplicate_line_xpm ) );

            msg = AddHotkeyName( _("Move Edge Exactly" ), g_Module_Editor_Hokeys_Descr, HK_MOVE_ITEM_EXACT );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EXACT, msg, KiBitmap( move_line_xpm ) );

            msg = AddHotkeyName( _("Create Edge Array" ), g_Module_Editor_Hokeys_Descr, HK_CREATE_ARRAY );
            AddMenuItem( PopMenu, ID_POPUP_PCB_CREATE_ARRAY, msg, KiBitmap( array_line_xpm ) );

            if( ( flags & (IS_NEW | IS_MOVED) ) == IS_MOVED )
                AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_EDGE, _( "Place edge" ),
                             KiBitmap( checked_ok_xpm ) );

            msg = AddHotkeyName( _("Edit" ), g_Module_Editor_Hokeys_Descr, HK_EDIT_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_MODEDIT_EDIT_BODY_ITEM,
                         msg, KiBitmap( options_segment_xpm  ) );

            msg = AddHotkeyName( _("Delete Edge" ), g_Module_Editor_Hokeys_Descr, HK_DELETE );
            AddMenuItem( PopMenu, ID_POPUP_PCB_DELETE_EDGE, msg, KiBitmap( delete_xpm ) );

            wxMenu* edit_global_mnu = new wxMenu;
            AddMenuItem( PopMenu, edit_global_mnu, ID_POPUP_MODEDIT_GLOBAL_EDIT_EDGE,
                         _( "Global Changes" ), KiBitmap( edit_xpm ) );
            AddMenuItem( edit_global_mnu, ID_POPUP_MODEDIT_EDIT_WIDTH_ALL_EDGE,
                         _( "Change Body Items Width" ), KiBitmap( width_segment_xpm ) );
            AddMenuItem( edit_global_mnu, ID_POPUP_MODEDIT_EDIT_LAYER_ALL_EDGE,
                         _( "Change Body Items Layer" ), KiBitmap( select_layer_pair_xpm ) );
        }
        break;

        case PCB_LINE_T:
        case PCB_TEXT_T:
        case PCB_VIA_T:
        case PCB_TRACE_T:
        case PCB_ZONE_T:
        case PCB_MARKER_T:
        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
            msg.Printf( wxT( "FOOTPRINT_EDIT_FRAME::OnRightClick Error: Unexpected DrawType %d" ),
                        item->Type() );
            DisplayError( this, msg );
            break;

        case SCREEN_T:
        case TYPE_NOT_INIT:
        case PCB_T:
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
    }

    if( ( GetToolId() == ID_MODEDIT_LINE_TOOL ) ||
        ( GetToolId() == ID_MODEDIT_CIRCLE_TOOL ) ||
        ( GetToolId() == ID_MODEDIT_ARC_TOOL ) )
    {
        AddMenuItem( PopMenu, ID_POPUP_MODEDIT_ENTER_EDGE_WIDTH, _("Set Line Width" ),
                     KiBitmap( width_segment_xpm ) );
        PopMenu->AppendSeparator();
    }

    return true;
}

/*
 * Called on a mouse left button double click
 */
void FOOTPRINT_EDIT_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* item = GetCurItem();

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( ( item == NULL ) || ( item->GetFlags() == 0 ) )
        {
            item = ModeditLocateAndDisplay();
        }

        if( ( item == NULL ) || ( item->GetFlags() != 0 ) )
            break;

        // Item found
        SetCurItem( item );
        OnEditItemRequest( DC, item );
        break;      // end case 0

    case ID_PCB_ADD_LINE_BUTT:
    {
        if( item && item->IsNew() )
        {
            End_Edge_Module( (EDGE_MODULE*) item );
            SetCurItem( NULL );
            m_canvas->Refresh();
        }

        break;
    }

    default:
        break;
    }
}


void FOOTPRINT_EDIT_FRAME::OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_PAD_T:
        InstallPadOptionsFrame( static_cast<D_PAD*>( aItem ) );
        m_canvas->MoveCursorToCrossHair();
        break;

    case PCB_MODULE_T:
        {
        DIALOG_MODULE_MODULE_EDITOR dialog( this, static_cast<MODULE*>( aItem ) );
        int ret = dialog.ShowModal();
        GetScreen()->GetCurItem()->ClearFlags();
        m_canvas->MoveCursorToCrossHair();

        if( ret > 0 )
            m_canvas->Refresh();
        }
        break;

    case PCB_MODULE_TEXT_T:
        InstallTextModOptionsFrame( static_cast<TEXTE_MODULE*>( aItem ), aDC );
        m_canvas->MoveCursorToCrossHair();
        break;

    case PCB_MODULE_EDGE_T :
        m_canvas->MoveCursorToCrossHair();
        InstallFootprintBodyItemPropertiesDlg( static_cast<EDGE_MODULE*>( aItem ) );
        m_canvas->Refresh();
        break;

    default:
        break;
    }
}
