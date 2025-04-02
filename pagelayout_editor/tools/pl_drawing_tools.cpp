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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>

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
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_DRAWING_TOOLS::PlaceItem( const TOOL_EVENT& aEvent )
{
    DS_DATA_ITEM::DS_ITEM_TYPE type = aEvent.Parameter<DS_DATA_ITEM::DS_ITEM_TYPE>();
    VECTOR2I                   cursorPos;
    DS_DRAW_ITEM_BASE*         item   = nullptr;
    bool                       isText = aEvent.IsAction( &PL_ACTIONS::placeText );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

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
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                item = nullptr;

                // There's nothing to roll-back, but we still need to pop the undo stack
                // This also deletes the item being placed.
                m_frame->RollbackFromUndo();
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
        cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() || ( item && evt->IsAction( &ACTIONS::undo ) )  )
        {
            if( item )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool placeItem = true;

            if( !item )
            {
                DS_DATA_ITEM* dataItem = m_frame->AddDrawingSheetItem( type );

                if( dataItem )  // dataItem = nullptr can happens if the command was cancelled
                {
                    m_frame->SaveCopyInUndoList();

                    m_toolMgr->RunAction( ACTIONS::selectionClear );

                    item = dataItem->GetDrawItems()[0];
                    item->SetFlags( IS_NEW | IS_MOVING );

                    // Select the item but don't inform other tools (to prevent the Properties
                    // panel from updating the item before it has been placed)
                    m_selectionTool->AddItemToSel( item, true );

                    // update the cursor so it looks correct before another event
                    setCursor();

                    // Text is a single-click-place; all others are first-click-creates,
                    // second-click-places.
                    placeItem = dataItem->GetType() == DS_DATA_ITEM::DS_TEXT;
                }
            }

            if( item && placeItem )
            {
                item->GetPeer()->MoveStartPointToIU( cursorPos );
                item->SetPosition( item->GetPeer()->GetStartPosIU( 0 ) );
                item->ClearEditFlags();
                getView()->Update( item );

                // Now we re-select and inform other tools, so that the Properties panel
                // is updated.
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_selectionTool->AddItemToSel( item, false );

                item = nullptr;

                m_frame->OnModify();
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
            item->GetPeer()->MoveStartPointToIU( cursorPos );
            item->SetPosition( item->GetPeer()->GetStartPosIU( 0 ) );
            getView()->Update( item );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is an item to be placed
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
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

    m_frame->PushTool( aEvent );

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
            }

            if( evt->IsPointEditor() || evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !item ) // start drawing
            {
                m_frame->SaveCopyInUndoList();
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                DS_DATA_ITEM* dataItem = m_frame->AddDrawingSheetItem( type );
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
    m_frame->PopTool( aEvent );
    return 0;
}


void PL_DRAWING_TOOLS::setTransitions()
{
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawLine.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawRectangle.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeText.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeImage.MakeEvent() );
}
