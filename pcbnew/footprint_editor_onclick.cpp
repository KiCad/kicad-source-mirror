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
 * @file footprint_editor_onclick.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gr_basic.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <origin_viewitem.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <tools/pcbnew_control.h>
#include <hotkeys.h>
#include <footprint_edit_frame.h>
#include <dialog_edit_footprint_for_fp_editor.h>
#include <menus_helpers.h>


void FOOTPRINT_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    BOARD_ITEM* item = GetCurItem();

    m_canvas->CrossHairOff( DC );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        if( item && item->GetEditFlags() ) // Move item command in progress
        {
            switch( item->Type() )
            {
            case PCB_MODULE_TEXT_T:
                PlaceTexteModule( static_cast<TEXTE_MODULE*>( item ), DC );
                break;

            case PCB_MODULE_EDGE_T:
                SaveCopyInUndoList( GetBoard()->m_Modules, UR_CHANGED );
                Place_EdgeMod( static_cast<EDGE_MODULE*>( item ) );
                break;

            case PCB_PAD_T:
                PlacePad( static_cast<D_PAD*>( item ), DC );
                break;

            default:
                wxLogDebug( wxT( "WinEDA_ModEditFrame::OnLeftClick err:Struct %d, m_Flag %X" ),
                            item->Type(), item->GetEditFlags() );
                item->ClearFlags();
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
    bool no_item_edited = item == NULL || item->GetEditFlags() == 0;

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
                wxLogDebug( wxT( "ProcessCommand error: unknown shape" ) );
        }
        break;

    case ID_MODEDIT_DELETE_TOOL:
        if( ! no_item_edited )    // Item in edit, cannot delete it
            break;

        item = ModeditLocateAndDisplay();

        if( item && item->Type() != PCB_MODULE_T ) // Cannot delete the module itself
        {
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_CHANGED );
            RemoveStruct( item );
            SetCurItem( NULL );
        }

        break;

    case ID_MODEDIT_ANCHOR_TOOL:
        {
            MODULE* module = GetBoard()->m_Modules;

            if( module == NULL || module->GetEditFlags() != 0 )
                break;

            SaveCopyInUndoList( module, UR_CHANGED );

            // set the new relative internal local coordinates of footprint items
            wxPoint moveVector = module->GetPosition() - GetCrossHairPosition();
            module->MoveAnchorPosition( moveVector );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            SetNoToolSelected();
            SetCurItem( NULL );
            m_canvas->Refresh();
        }
        break;

    case ID_MODEDIT_PLACE_GRID_COORD:
        PCBNEW_CONTROL::SetGridOrigin( GetGalCanvas()->GetView(), this,
                                       new KIGFX::ORIGIN_VIEWITEM( GetBoard()->GetGridOrigin(), UR_TRANSIENT ),
                                       GetCrossHairPosition() );
        m_canvas->Refresh();
        break;

    case ID_MODEDIT_TEXT_TOOL:
        if( GetBoard()->m_Modules == NULL )
            break;

        SaveCopyInUndoList( GetBoard()->m_Modules, UR_CHANGED );
        CreateTextModule( GetBoard()->m_Modules, DC );
        break;

    case ID_MODEDIT_PAD_TOOL:
        if( GetBoard()->m_Modules )
        {
            SaveCopyInUndoList( GetBoard()->m_Modules, UR_CHANGED );
            AddPad( GetBoard()->m_Modules, true );
        }

        break;

    case ID_MODEDIT_MEASUREMENT_TOOL:
        DisplayError( this, _( "Measurement Tool not available in Legacy Toolset" ) );
        SetNoToolSelected();
        break;

    default:
        wxLogDebug( wxT( "FOOTPRINT_EDIT_FRAME::ProcessCommand error" ) );
        SetNoToolSelected();
    }

    m_canvas->CrossHairOn( DC );
}


bool FOOTPRINT_EDIT_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
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
        if( item == NULL || item->GetEditFlags() == 0 )
            item = ModeditLocateAndDisplay();

        if( item == NULL || item->GetEditFlags() != 0 )
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
        editFootprintProperties( (MODULE*) aItem );
        m_canvas->MoveCursorToCrossHair();
        m_canvas->Refresh();
        break;

    case PCB_MODULE_TEXT_T:
        InstallTextOptionsFrame( aItem, aDC );
        break;

    case PCB_MODULE_EDGE_T :
        InstallGraphicItemPropertiesDialog( aItem );
        break;

    default:
        break;
    }
}
