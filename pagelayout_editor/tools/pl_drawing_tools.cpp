/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pl_editor_frame.h>
#include <class_draw_panel_gal.h>
#include <pl_editor_id.h>
#include <confirm.h>
#include <view/view_group.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/pl_actions.h>
#include <tools/pl_selection_tool.h>
#include <tools/pl_drawing_tools.h>
#include <bitmaps.h>
#include <ws_draw_item.h>
#include <ws_data_item.h>
#include <invoke_pl_editor_dialog.h>
#include "pl_point_editor.h"

PL_DRAWING_TOOLS::PL_DRAWING_TOOLS() :
        TOOL_INTERACTIVE( "plEditor.InteractiveDrawing" ),
        m_frame( nullptr ),
        m_selectionTool( nullptr )
{
}


bool PL_DRAWING_TOOLS::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    // Finally, add the standard zoom/grid items
    m_frame->AddStandardSubMenus( m_menu );

    return true;
}


void PL_DRAWING_TOOLS::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_DRAWING_TOOLS::PlaceItem( const TOOL_EVENT& aEvent )
{
    WS_DATA_ITEM::WS_ITEM_TYPE type = aEvent.Parameter<WS_DATA_ITEM::WS_ITEM_TYPE>();
    VECTOR2I                   cursorPos;
    WS_DRAW_ITEM_BASE*         item = nullptr;

    m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( item ? wxCURSOR_ARROW : wxCURSOR_PENCIL );
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        auto cleanup = [&] () {
            m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
            delete item;
            item = nullptr;

            // There's nothing to roll-back, but we still need to pop the undo stack
            m_frame->RollbackFromUndo();
        };

        if( evt->IsCancelInteractive() )
        {
            if( item )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( item )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // First click creates...
            if( !item )
            {
                WS_DATA_ITEM* dataItem = m_frame->AddPageLayoutItem( type );

                if( dataItem )  // dataItem = nullptr can happens if the command was cancelled
                {
                    m_frame->SaveCopyInUndoList();

                    m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

                    item = dataItem->GetDrawItems()[0];
                    item->SetFlags( IS_NEW | IS_MOVED );
                    m_selectionTool->AddItemToSel( item );
                }
            }
            // ... and second click places:
            else
            {
                item->GetPeer()->MoveStartPointToUi( (wxPoint) cursorPos );
                item->SetPosition( item->GetPeer()->GetStartPosUi( 0 ) );
                item->ClearEditFlags();
                getView()->Update( item );

                item = nullptr;

                m_frame->OnModify();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->GetPeer()->MoveStartPointToUi( (wxPoint) cursorPos );
            item->SetPosition( item->GetPeer()->GetStartPosUi( 0 ) );
            getView()->Update( item );
        }
        else
            evt->SetPassEvent();

        // Enable autopanning and cursor capture only when there is an item to be placed
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    return 0;
}


int PL_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    PL_POINT_EDITOR*           pointEditor = m_toolMgr->GetTool<PL_POINT_EDITOR>();
    WS_DATA_ITEM::WS_ITEM_TYPE type = aEvent.Parameter<WS_DATA_ITEM::WS_ITEM_TYPE>();
    WS_DRAW_ITEM_BASE*         item = nullptr;

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !pointEditor->HasPoint() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

            if( item )
            {
                item = nullptr;
                m_frame->RollbackFromUndo();
            }
            else if( evt->IsCancelInteractive() )
            {
                break;
            }

            if( evt->IsActivate() && !evt->IsPointEditor() && !evt->IsMoveTool() )
                break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !item ) // start drawing
            {
                m_frame->SaveCopyInUndoList();
                m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

                WS_DATA_ITEM* dataItem = m_frame->AddPageLayoutItem( type );
                dataItem->MoveToUi( (wxPoint) cursorPos );

                item = dataItem->GetDrawItems()[0];
                item->SetFlags( IS_NEW );
                m_selectionTool->AddItemToSel( item );
            }
            else    // finish drawing
            {
                item->ClearEditFlags();
                item = nullptr;
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );

                m_frame->OnModify();
            }
        }

        else if( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() )
        {
            if( item )
            {
                item->GetPeer()->MoveEndPointToUi( (wxPoint) cursorPos );
                item->SetEnd( item->GetPeer()->GetEndPosUi( 0 ) );
                getView()->Update( item );
            }
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        else
            evt->SetPassEvent();

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    m_frame->PopTool( tool );
    return 0;
}


void PL_DRAWING_TOOLS::setTransitions()
{
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawLine.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawRectangle.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeText.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeImage.MakeEvent() );
}
