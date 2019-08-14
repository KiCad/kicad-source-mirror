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

#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <ee_actions.h>
#include <bitmaps.h>
#include <base_struct.h>
#include <sch_item.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include "sch_move_tool.h"


// For adding to or removing from selections
#define QUIET_MODE true


SCH_MOVE_TOOL::SCH_MOVE_TOOL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveMove" ),
        m_moveInProgress( false ),
        m_isDragOperation( false ),
        m_moveOffset( 0, 0 )
{
}


bool SCH_MOVE_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto moveCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    // Add move actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::move, moveCondition, 150 );
    selToolMenu.AddItem( EE_ACTIONS::drag, moveCondition, 150 );

    return true;
}


/* TODO - Tom/Jeff
  - add preferences option "Move origin: always cursor / item origin"
  - add preferences option "Default drag action: drag items / move"
  - add preferences option "Drag always selects"
  */

#define STD_VECTOR_REMOVE( v, item ) v.erase( std::remove( v.begin(), v.end(), item ), v.end() )


int SCH_MOVE_TOOL::Main( const TOOL_EVENT& aEvent )
{
    const KICAD_T movableItems[] =
    {
        SCH_MARKER_T,
        SCH_JUNCTION_T,
        SCH_NO_CONNECT_T,
        SCH_BUS_BUS_ENTRY_T,
        SCH_BUS_WIRE_ENTRY_T,
        SCH_LINE_T,
        SCH_BITMAP_T,
        SCH_TEXT_T,
        SCH_LABEL_T,
        SCH_GLOBAL_LABEL_T,
        SCH_HIER_LABEL_T,
        SCH_FIELD_T,
        SCH_COMPONENT_T,
        SCH_SHEET_PIN_T,
        SCH_SHEET_T,
        EOT
    };

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    controls->SetSnapping( true );

    m_anchorPos.reset();

    if( aEvent.IsAction( &EE_ACTIONS::move ) )
        m_isDragOperation = false;
    else if( aEvent.IsAction( &EE_ACTIONS::drag ) )
        m_isDragOperation = true;
    else if( aEvent.IsAction( &EE_ACTIONS::moveActivate ) )
        m_isDragOperation = !m_frame->GetDragActionIsMove();
    else
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );

    if( m_moveInProgress )
    {
        auto sel = m_selectionTool->GetSelection().Front();

        if( sel && !sel->IsNew() )
        {
            // User must have switched from move to drag or vice-versa.  Reset the selected
            // items so we can start again with the current m_isDragOperation.
            m_frame->RollbackSchematicFromUndo();
            m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
            m_anchorPos = m_cursor - m_moveOffset;
            m_moveInProgress = false;
            controls->SetAutoPan( false );

            // And give it a kick so it doesn't have to wait for the first mouse movement to
            // refresh.
            m_toolMgr->RunAction( EE_ACTIONS::restartMove );
        }

        return 0;
    }

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    EE_SELECTION& selection = m_selectionTool->RequestSelection( movableItems );
    bool          unselect = selection.IsHover();

    if( selection.Empty() )
        return 0;

    Activate();
    controls->ShowCursor( true );

    bool        restore_state = false;
    bool        chain_commands = false;
    TOOL_EVENT* evt = const_cast<TOOL_EVENT*>( &aEvent );
    VECTOR2I    prevPos;

    m_cursor = controls->GetCursorPosition();

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &EE_ACTIONS::moveActivate ) || evt->IsAction( &EE_ACTIONS::restartMove )
                || evt->IsAction( &EE_ACTIONS::move ) || evt->IsAction( &EE_ACTIONS::drag )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                SCH_ITEM* sch_item = (SCH_ITEM*) selection.Front();
                bool      appendUndo = sch_item && sch_item->IsNew();

                //------------------------------------------------------------------------
                // Setup a drag or a move
                //
                for( SCH_ITEM* it = m_frame->GetScreen()->GetDrawItems(); it; it = it->Next() )
                {
                    if( !it->IsSelected() )
                        it->ClearFlags( STARTPOINT | ENDPOINT | SELECTEDNODE );

                    if( !selection.IsHover() && it->IsSelected() )
                        it->SetFlags( STARTPOINT | ENDPOINT );
                }

                if( m_isDragOperation )
                {
                    // Add connections to the selection for a drag.
                    //
                    for( EDA_ITEM* item : selection )
                    {
                        if( static_cast<SCH_ITEM*>( item )->IsConnectable() )
                        {
                            std::vector<wxPoint> connections;

                            if( item->Type() == SCH_LINE_T )
                                static_cast<SCH_LINE*>( item )->GetSelectedPoints( connections );
                            else
                                static_cast<SCH_ITEM*>( item )->GetConnectionPoints( connections );

                            for( wxPoint point : connections )
                                getConnectedDragItems( (SCH_ITEM*) item, point, m_dragAdditions );
                        }
                    }

                    m_selectionTool->AddItemsToSel( &m_dragAdditions, QUIET_MODE );
                }
                else
                {
                    // Mark the edges of the block with dangling flags for a move.
                    //
                    std::vector<DANGLING_END_ITEM> internalPoints;

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->GetEndPoints( internalPoints );

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( internalPoints );
                }
                // Generic setup
                //
                for( EDA_ITEM* item : selection )
                {
                    if( item->IsNew() )
                    {
                        if( ( item->GetFlags() & SELECTEDNODE ) != 0 && m_isDragOperation )
                        {
                            // Item was added in getConnectedDragItems
                            saveCopyInUndoList( (SCH_ITEM*) item, UR_NEW, appendUndo );
                            STD_VECTOR_REMOVE( m_dragAdditions, item );
                            appendUndo = true;
                        }
                        else
                        {
                            // Item was added in a previous command (and saved to undo by
                            // that command)
                        }
                    }
                    else if( item->GetParent() && item->GetParent()->IsSelected() )
                    {
                        // Item will be (or has been) saved to undo by parent
                    }
                    else
                    {
                        saveCopyInUndoList( (SCH_ITEM*) item, UR_CHANGED, appendUndo );
                        appendUndo = true;
                    }

                    SCH_ITEM* schItem = (SCH_ITEM*) item;
                    schItem->SetStoredPos( schItem->GetPosition() );
                }

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( evt->IsAction( &EE_ACTIONS::restartMove ) )
                {
                    wxASSERT_MSG( m_anchorPos, "Should be already set from previous cmd" );
                }
                else if( selection.Front()->GetFlags() & IS_NEW )
                {
                    m_anchorPos = selection.GetReferencePoint();
                }

                if( m_anchorPos )
                {
                    VECTOR2I delta = m_cursor - (*m_anchorPos);

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta, m_isDragOperation );
                        updateView( item );
                    }

                    m_anchorPos = m_cursor;
                }
                // For some items, moving the cursor to anchor is not good (for instance large
                // hierarchical sheets or components can have the anchor outside the view)
                else if( selection.Size() == 1 && sch_item->IsMovableFromAnchorPoint()
                         && m_frame->GetMoveWarpsCursor() )
                {
                    if( sch_item->Type() == SCH_LINE_T && !( sch_item->GetFlags() & STARTPOINT ) )
                        m_anchorPos = static_cast<SCH_LINE*>( sch_item )->GetEndPoint();
                    else
                        m_anchorPos = sch_item->GetPosition();

                    getViewControls()->WarpCursor( *m_anchorPos, true, true );
                    m_cursor = *m_anchorPos;
                }
                // ...otherwise modify items with regard to the grid-snapped cursor position
                else
                {
                    m_cursor = getViewControls()->GetCursorPosition( true );
                    m_anchorPos = m_cursor;
                }

                controls->SetCursorPosition( m_cursor, false );
                m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = controls->GetCursorPosition();
            VECTOR2I delta( m_cursor - prevPos );
            m_anchorPos = m_cursor;

            m_moveOffset += delta;
            prevPos = m_cursor;

            for( EDA_ITEM* item : selection )
            {
                // Don't double move pins, fields, etc.
                if( item->GetParent() && item->GetParent()->IsSelected() )
                    continue;

                moveItem( item, delta, m_isDragOperation );
                updateView( item );
            }

            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_frame->UpdateMsgPanel();
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive() )
        {
            if( m_moveInProgress )
                restore_state = true;

            break;
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &ACTIONS::doDelete ) )
            {
                // Exit on a remove operation; there is no further processing for removed items.
                break;
            }
            else if( evt->IsAction( &ACTIONS::duplicate ) )
            {
                if( selection.Front()->IsNew() )
                {
                    // This doesn't really make sense; we'll just end up dragging a stack of
                    // objects so Duplicate() is going to ignore this and we'll just carry on.
                    continue;
                }

                // Move original back and exit.  The duplicate will run in its own loop.
                restore_state = true;
                unselect = false;
                chain_commands = true;
                break;
            }
            else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                    && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_CMP_MAX )
                {
                    SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( selection.Front() );
                    int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                    if( component )
                    {
                        m_frame->SelectUnit( component, unit );
                        m_toolMgr->RunAction( ACTIONS::refreshPreview );
                    }
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }
        else
            evt->SetPassEvent();

        controls->SetAutoPan( m_moveInProgress );

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    m_anchorPos.reset();

    for( EDA_ITEM* item : selection )
        item->ClearEditFlags();

    if( restore_state )
    {
        m_frame->RollbackSchematicFromUndo();

        if( unselect )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
        else
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
    else
    {
        m_toolMgr->RunAction( EE_ACTIONS::addNeededJunctions, true, &selection );
        m_frame->SchematicCleanUp();
        m_frame->TestDanglingEnds();

        if( unselect )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
        else
            m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );

        m_frame->OnModify();
    }

    m_dragAdditions.clear();
    m_moveInProgress = false;
    m_frame->PopTool( tool );
    return 0;
}


void SCH_MOVE_TOOL::getConnectedDragItems( SCH_ITEM* aOriginalItem, wxPoint aPoint,
                                           EDA_ITEMS& aList )
{
    for( SCH_ITEM* test = m_frame->GetScreen()->GetDrawItems(); test; test = test->Next() )
    {
        if( test->IsSelected() || !test->IsConnectable() || !test->CanConnect( aOriginalItem ) )
            continue;

        switch( test->Type() )
        {
        case SCH_LINE_T:
        {
            // Select the connected end of wires/bus connections.
            SCH_LINE* testLine = (SCH_LINE*) test;

            if( testLine->GetStartPoint() == aPoint )
            {
                if( !( testLine->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( testLine );

                testLine->SetFlags( STARTPOINT | SELECTEDNODE );
            }
            else if( testLine->GetEndPoint() == aPoint )
            {
                if( !( testLine->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( testLine );

                testLine->SetFlags( ENDPOINT | SELECTEDNODE );
            }
            break;
        }

        case SCH_SHEET_T:
            // Dragging a sheet just because it's connected to something else feels a bit like
            // the tail wagging the dog, but this could be moved down to the next case.
            break;

        case SCH_COMPONENT_T:
            if( test->IsConnected( aPoint ) )
            {
                // Add a new wire between the component and the selected item so the selected
                // item can be dragged.
                SCH_LINE* newWire = new SCH_LINE( aPoint, LAYER_WIRE );
                newWire->SetFlags( IS_NEW );
                m_frame->AddToScreen( newWire, m_frame->GetScreen() );

                newWire->SetFlags( SELECTEDNODE | STARTPOINT );
                aList.push_back( newWire );
            }
            break;

        case SCH_NO_CONNECT_T:
        case SCH_JUNCTION_T:
            // Select no-connects and junctions that are connected to items being moved.
            if( test->IsConnected( aPoint ) )
                aList.push_back( test );

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Select labels and bus entries that are connected to a wire being moved.
            if( aOriginalItem->Type() == SCH_LINE_T )
            {
                std::vector<wxPoint> connections;
                test->GetConnectionPoints( connections );

                for( wxPoint& point : connections )
                {
                    if( aOriginalItem->HitTest( point ) )
                        aList.push_back( test );
                }
            }
            break;

        default:
            break;
        }
    }
}


void SCH_MOVE_TOOL::moveItem( EDA_ITEM* aItem, VECTOR2I aDelta, bool isDrag )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
        if( aItem->GetFlags() & STARTPOINT )
            static_cast<SCH_LINE*>( aItem )->MoveStart( (wxPoint) aDelta );

        if( aItem->GetFlags() & ENDPOINT )
            static_cast<SCH_LINE*>( aItem )->MoveEnd( (wxPoint) aDelta );

        break;

    case SCH_PIN_T:
    case SCH_FIELD_T:
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) aItem->GetParent();
        TRANSFORM      transform = component->GetTransform().InverseTransform();
        wxPoint        transformedDelta = transform.TransformCoordinate( (wxPoint) aDelta );

        static_cast<SCH_ITEM*>( aItem )->Move( transformedDelta );
        break;
    }
    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN* pin = (SCH_SHEET_PIN*) aItem;
        pin->SetStoredPos( pin->GetStoredPos() + (wxPoint) aDelta );
        pin->ConstrainOnEdge( pin->GetStoredPos() );
        break;
    }
    default:
        static_cast<SCH_ITEM*>( aItem )->Move( (wxPoint) aDelta );
        break;
    }

    aItem->SetFlags( IS_MOVED );
}


void SCH_MOVE_TOOL::setTransitions()
{
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::moveActivate.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::move.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::drag.MakeEvent() );
}
