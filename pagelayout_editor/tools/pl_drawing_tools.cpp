/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


// Drawing tool actions
TOOL_ACTION PL_ACTIONS::drawLine( "plEditor.InteractiveDrawing.drawLine",
        AS_GLOBAL, 0, "",
        _( "Add Line" ), _( "Add a line" ),
        add_graphical_segments_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::drawRectangle( "plEditor.InteractiveDrawing.drawRectangle",
        AS_GLOBAL, 0, "",
        _( "Add Rectangle" ), _( "Add a rectangle" ),
        add_rectangle_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::placeText( "plEditor.InteractiveDrawing.placeText",
        AS_GLOBAL, 0, "",
        _( "Add Text" ), _( "Add a text item" ),
        text_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::placeImage( "plEditor.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, "",
        _( "Add Bitmap" ), _( "Add a bitmap image" ),
        image_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::addLine( "plEditor.InteractiveDrawing.addLine",
        AS_GLOBAL, 0, "",
        _( "Add Line" ), _( "Add a line" ),
        add_dashed_line_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::addRectangle( "plEditor.InteractiveDrawing.addRectangle",
        AS_GLOBAL, 0, "",
        _( "Add Rectangle" ), _( "Add a rectangle" ),
        add_rectangle_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::addText( "plEditor.InteractiveDrawing.addText",
        AS_GLOBAL, 0, "",
        _( "Add Text" ), _( "Add a text item" ),
        text_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::addImage( "plEditor.InteractiveDrawing.addImage",
        AS_GLOBAL, 0, "",
        _( "Add Bitmap" ), _( "Add a bitmap image" ),
        image_xpm, AF_ACTIVATE );


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
    ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways, 1 );

    // Finally, add the standard zoom/grid items
    m_menu.AddStandardSubMenus( m_frame );

    return true;
}


void PL_DRAWING_TOOLS::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_DRAWING_TOOLS::PlaceItem( const TOOL_EVENT& aEvent )
{
    bool isText;
    bool isImmediate = false;

    if( aEvent.IsAction( &PL_ACTIONS::placeText ) )
    {
        isText = true;
        m_frame->SetToolID( ID_PL_TEXT_TOOL, wxCURSOR_PENCIL, _( "Add text" ) );
    }
    else if( aEvent.IsAction( &PL_ACTIONS::placeImage ) )
    {
        isText = false;
        m_frame->SetToolID( ID_PL_IMAGE_TOOL, wxCURSOR_PENCIL, _( "Add image" ) );
    }
    else if( aEvent.IsAction( & PL_ACTIONS::addText ) )
    {
        isText = true;
        isImmediate = true;
    }
    else if( aEvent.IsAction( & PL_ACTIONS::addImage ) )
    {
        isText = false;
        isImmediate = true;
    }
    else
        wxCHECK_MSG( false, 0, "Unknown action in PL_DRAWING_TOOLS::PlaceItem()" );

    VECTOR2I           cursorPos;
    WS_DRAW_ITEM_BASE* item = nullptr;

    m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
                delete item;
                item = nullptr;

                // There's nothing to roll-back, but we still need to pop the undo stack
                m_frame->RollbackFromUndo();

                if( !evt->IsActivate() && !isImmediate )
                    continue;
            }

            break;
        }

        else if( evt->IsClick( BUT_LEFT ) || ( isImmediate && !item ) )
        {
            // First click creates...
            if( !item )
            {
                m_frame->SaveCopyInUndoList();

                m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

                WS_DATA_ITEM* dataItem;
                dataItem = m_frame->AddPageLayoutItem( isText ? WS_DATA_ITEM::WS_TEXT
                                                              : WS_DATA_ITEM::WS_BITMAP );
                item = dataItem->GetDrawItems()[0];
                item->SetFlags( IS_NEW | IS_MOVED );
                m_selectionTool->AddItemToSel( item );
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

                if( isImmediate )
                    break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        else if( item && ( evt->IsAction( &PL_ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->GetPeer()->MoveStartPointToUi( (wxPoint) cursorPos );
            item->SetPosition( item->GetPeer()->GetStartPosUi( 0 ) );
            getView()->Update( item );
        }

        // Enable autopanning and cursor capture only when there is an item to be placed
        getViewControls()->SetAutoPan( !!item );
        getViewControls()->CaptureCursor( !!item );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int PL_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    bool isDrawLine;
    bool isImmediate = false;

    if( aEvent.IsAction( &PL_ACTIONS::drawLine ) )
    {
        isDrawLine = true;
        m_frame->SetToolID( ID_PL_LINE_TOOL, wxCURSOR_PENCIL, _( "Draw line" ) );
    }
    else if( aEvent.IsAction( &PL_ACTIONS::drawRectangle ) )
    {
        isDrawLine = false;
        m_frame->SetToolID( ID_PL_RECTANGLE_TOOL, wxCURSOR_PENCIL, _( "Draw rectangle" ) );
    }
    else if( aEvent.IsAction( &PL_ACTIONS::addLine ) )
    {
        isDrawLine = true;
        isImmediate = true;
    }
    else if( aEvent.IsAction( &PL_ACTIONS::addRectangle ) )
    {
        isDrawLine = false;
        isImmediate = true;
    }
    else
        wxCHECK_MSG( false, 0, "Unknown action in PL_DRAWING_TOOLS::DrawShape()" );

    m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    Activate();

    WS_DRAW_ITEM_BASE* item = nullptr;

    // Main loop: keep receiving events
    while( auto evt = Wait() )
    {
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

            if( item )
            {
                item = nullptr;
                m_frame->RollbackFromUndo();

                if( !evt->IsActivate() && !isImmediate )
                    continue;
            }

            break;
        }

        else if( evt->IsClick( BUT_LEFT ) || ( isImmediate && !item ) )
        {
            if( !item ) // start drawing
            {
                m_frame->SaveCopyInUndoList();
                m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

                WS_DATA_ITEM::WS_ITEM_TYPE dataType;

                if( isDrawLine )
                    dataType = WS_DATA_ITEM::WS_SEGMENT;
                else
                    dataType = WS_DATA_ITEM::WS_RECT;

                WS_DATA_ITEM* dataItem = m_frame->AddPageLayoutItem( dataType );
                dataItem->MoveToUi( (wxPoint) cursorPos );

                item = dataItem->GetDrawItems()[0];
                item->SetFlags( IS_NEW );
                m_selectionTool->AddItemToSel( item );
            }
            else    // finish drawing
            {
                item->ClearEditFlags();
                item = nullptr;

                m_frame->OnModify();

                if( isImmediate )
                {
                    m_toolMgr->RunAction( ACTIONS::activatePointEditor );
                    break;
                }
            }
        }

        else if( evt->IsAction( &PL_ACTIONS::refreshPreview ) || evt->IsMotion() )
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

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( !!item );
        getViewControls()->CaptureCursor( !!item );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


void PL_DRAWING_TOOLS::setTransitions()
{
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawLine.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::drawRectangle.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeText.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::placeImage.MakeEvent() );

    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::addLine.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::DrawShape,           PL_ACTIONS::addRectangle.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::addText.MakeEvent() );
    Go( &PL_DRAWING_TOOLS::PlaceItem,           PL_ACTIONS::addImage.MakeEvent() );
}
