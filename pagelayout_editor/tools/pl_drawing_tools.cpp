/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>

#include "pl_editor_frame.h"
#include "tools/pl_actions.h"
#include "tools/pl_selection_tool.h"
#include "tools/pl_drawing_tools.h"
#include "pgm_base.h"

PL_DRAWING_TOOLS::PL_DRAWING_TOOLS() :
        TOOL_INTERACTIVE( "plEditor.InteractiveDrawing" ),
        m_frame( nullptr ),
        m_selectionTool( nullptr )
{
}


PL_DRAWING_TOOLS::~PL_DRAWING_TOOLS()
{
}


bool PL_DRAWING_TOOLS::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();

    auto& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    // Finally, add the standard zoom/grid items
    m_frame->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void PL_DRAWING_TOOLS::Reset( RESET_REASON aReason )
{
    if( aReason != REDRAW )
        m_pendingItem.reset();

    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_DRAWING_TOOLS::PlaceItem( const TOOL_EVENT& aEvent )
{
    DS_DATA_ITEM::DS_ITEM_TYPE type = aEvent.Parameter<DS_DATA_ITEM::DS_ITEM_TYPE>();
    VECTOR2I                   cursorPos;
    bool                       isText = aEvent.IsAction( &PL_ACTIONS::placeText );
    DS_DATA_ITEM*              item = nullptr;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    TOOL_EVENT         originalEvent = aEvent;          // This can change out from under us when the event loop runs
    SCOPED_TOOL_PUSHER raii( m_frame, originalEvent );

    auto setCursor =
            [&]()
            {
                if( item )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
                else if( isText )
                {
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TEXT );
                }
                else if( aEvent.IsAction( &PL_ACTIONS::placeImage ) )
                {
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
                }
                else
                {
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
                }
            };

    auto cleanup =
            [&]()
            {
                getView()->Remove( item->GetDrawItems()[0] );
                item = nullptr;
                m_pendingItem.reset();
            };

    auto createPending =
            [&]() -> bool
            {
                m_pendingItem.reset( m_frame->CreateDrawingSheetItem( type ) );
                item = m_pendingItem.get();

                if( !item )
                    return false;

                item->MoveToIU( getViewControls()->GetCursorPosition() );

                DS_DRAW_ITEM_BASE* drawItem = item->GetDrawItems()[0];
                drawItem->SetFlags( IS_NEW | IS_MOVING );
                getView()->Update( drawItem );

                setCursor();
                return true;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    // Set initial cursor
    setCursor();

    if( isText )
        createPending();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() || ( !isText && item && evt->IsAction( &ACTIONS::undo ) ) )
        {
            bool wasPending = item != nullptr;

            if( wasPending )
                cleanup();

            if( isText || !wasPending )
                break;
        }
        else if( evt->IsActivate() )
        {
            if( item )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // Make sure we come back after the move tool runs
                m_frame->PushTool( originalEvent );
            }

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool placeItem = item != nullptr;

            if( !item )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                if( createPending() )
                {
                    // Text is a single-click-place. All others are first-click-creates,
                    // second-click-places.
                    placeItem = isText;
                }
            }

            if( item && placeItem )
            {
                m_frame->SaveCopyInUndoList();
                DS_DATA_MODEL::GetTheInstance().Append( m_pendingItem.release() );

                item->MoveStartPointToIU( cursorPos );

                DS_DRAW_ITEM_BASE* drawItem = item->GetDrawItems()[0];
                drawItem->SetPosition( item->GetStartPosIU( 0 ) );
                drawItem->ClearEditFlags();

                // A full canvas rebuild drops the pending item from the view, so a plain
                // view update is not enough to bring it back
                getView()->Remove( drawItem );
                getView()->Add( drawItem );

                // Now we select and inform other tools, so that the Properties panel
                // is updated.
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_selectionTool->AddItemToSel( drawItem, false );

                item = nullptr;

                m_frame->OnModify();

                if( isText )
                    createPending();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->MoveStartPointToIU( cursorPos );

            DS_DRAW_ITEM_BASE* drawItem = item->GetDrawItems()[0];
            drawItem->SetPosition( item->GetStartPosIU( 0 ) );

            getView()->Remove( drawItem );
            getView()->Add( drawItem );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only for two click placements
        getViewControls()->SetAutoPan( !isText && item != nullptr );
        getViewControls()->CaptureCursor( !isText && item != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int PL_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    DS_DATA_ITEM::DS_ITEM_TYPE type = aEvent.Parameter<DS_DATA_ITEM::DS_ITEM_TYPE>();
    DS_DRAW_ITEM_BASE*         item = nullptr;

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    TOOL_EVENT         originalEvent = aEvent;          // This can change out from under us when the event loop runs
    SCOPED_TOOL_PUSHER raii( m_frame, originalEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() || ( item && evt->IsAction( &ACTIONS::undo ) )  )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            if( item )
            {
                item = nullptr;

                // Pop the undo stack and delete the item being placed
                m_frame->RollbackFromUndo();
            }
            else
            {
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( item )
            {
                item = nullptr;

                // Pop the undo stack and delete the item being placed
                m_frame->RollbackFromUndo();
                continue;
            }

            if( evt->IsPointEditor() || evt->IsMoveTool() )
            {
                // Make sure we come back after the move tool is done
                m_frame->PushTool( originalEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !item ) // start drawing
            {
                m_frame->SaveCopyInUndoList();
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                DS_DATA_ITEM* dataItem = m_frame->CreateDrawingSheetItem( type );
                DS_DATA_MODEL::GetTheInstance().Append( dataItem );
                dataItem->MoveToIU( cursorPos );

                item = dataItem->GetDrawItems()[0];
                item->SetFlags( IS_NEW );

                // Select the item but don't inform other tools (to prevent the Properties
                // panel from updating the item before it has been placed)
                m_selectionTool->AddItemToSel( item, true );
            }
            else    // finish drawing
            {
                // Now we re-select and inform other tools, so that the Properties panel
                // is updated.
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_selectionTool->AddItemToSel( item, false );

                item->ClearEditFlags();
                item = nullptr;

                // Activate point editor immediately to allow resizing of the item just created
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );

                m_frame->OnModify();
            }
        }
        else if( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() )
        {
            if( item )
            {
                item->GetPeer()->MoveEndPointToIU( cursorPos );
                item->SetEnd( item->GetPeer()->GetEndPosIU( 0 ) );
                getView()->Update( item );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


void PL_DRAWING_TOOLS::setTransitions()
{
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawLine.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawRectangle.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeText.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeImage.MakeEvent() );
}
