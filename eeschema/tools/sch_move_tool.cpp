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

#include <cmath>
#include <wx/log.h>
#include <trigo.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <sch_actions.h>
#include <sch_commit.h>
#include <eda_item.h>
#include <sch_group.h>
#include <sch_item.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <junction_helpers.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <view/view_controls.h>
#include <settings/settings_manager.h>
#include "sch_move_tool.h"


// For adding to or removing from selections
#define QUIET_MODE true


SCH_MOVE_TOOL::SCH_MOVE_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveMove" ),
        m_inMoveTool( false ),
        m_moveInProgress( false ),
        m_isDrag( false ),
        m_moveOffset( 0, 0 )
{
}


bool SCH_MOVE_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    auto moveCondition =
            []( const SELECTION& aSel )
            {
                if( aSel.Empty() || SELECTION_CONDITIONS::OnlyTypes( { SCH_MARKER_T } )( aSel ) )
                    return false;

                if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
                    return false;

                return true;
            };

    // Add move actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::move, moveCondition, 150 );
    selToolMenu.AddItem( SCH_ACTIONS::drag, moveCondition, 150 );
    selToolMenu.AddItem( SCH_ACTIONS::alignToGrid, moveCondition, 150 );

    return true;
}


void SCH_MOVE_TOOL::orthoLineDrag( SCH_COMMIT* aCommit, SCH_LINE* line, const VECTOR2I& splitDelta,
                                   int& xBendCount, int& yBendCount, const EE_GRID_HELPER& grid )
{
    // If the move is not the same angle as this move,  then we need to do something special with
    // the unselected end to maintain orthogonality. Either drag some connected line that is the
    // same angle as the move or add two lines to make a 90 degree connection
    if( !EDA_ANGLE( splitDelta ).IsParallelTo( line->Angle() ) || line->GetLength() == 0 )
    {
        VECTOR2I unselectedEnd = line->HasFlag( STARTPOINT ) ? line->GetEndPoint()
                                                             : line->GetStartPoint();
        VECTOR2I selectedEnd = line->HasFlag( STARTPOINT ) ? line->GetStartPoint()
                                                           : line->GetEndPoint();

        // Look for pre-existing lines we can drag with us instead of creating new ones
        bool      foundAttachment = false;
        bool      foundJunction   = false;
        bool      foundPin        = false;
        SCH_LINE* foundLine       = nullptr;

        for( EDA_ITEM* cItem : m_lineConnectionCache[line] )
        {
            foundAttachment = true;

            // If the move is the same angle as a connected line, we can shrink/extend that line
            // endpoint
            switch( cItem->Type() )
            {
            case SCH_LINE_T:
            {
                SCH_LINE* cLine = static_cast<SCH_LINE*>( cItem );

                // A matching angle on a non-zero-length line means lengthen/shorten will work
                if( EDA_ANGLE( splitDelta ).IsParallelTo( cLine->Angle() )
                        && cLine->GetLength() != 0 )
                {
                    foundLine = cLine;
                }

                // Zero length lines are lines that this algorithm has shortened to 0 so they also
                // work but we should prefer using a segment with length and angle matching when
                // we can (otherwise the zero length line will draw overlapping segments on them)
                if( !foundLine && cLine->GetLength() == 0 )
                    foundLine = cLine;

                break;
            }
            case SCH_JUNCTION_T:
                foundJunction = true;
                break;

            case SCH_PIN_T:
                foundPin = true;
                break;

            case SCH_SHEET_T:
                for( const auto& pair : m_specialCaseSheetPins )
                {
                    if( pair.first->IsConnected( selectedEnd ) )
                    {
                        foundPin = true;
                        break;
                    }
                }

                break;

            default:
                break;
            }
        }

        // Ok... what if our original line is length zero from moving in its direction, and the
        // last added segment of the 90 bend we are connected to is zero from moving it in its
        // direction after it was added?
        //
        // If we are moving in original direction, we should lengthen the original drag wire.
        // Otherwise we should lengthen the new wire.
        bool preferOriginalLine = false;

        if( foundLine
                && foundLine->GetLength() == 0
                && line->GetLength() == 0
                && EDA_ANGLE( splitDelta ).IsParallelTo( line->GetStoredAngle() ) )
        {
            preferOriginalLine = true;
        }
        // If we have found an attachment, but not a line, we want to check if it's a junction.
        // These are special-cased and get a single line added instead of a 90-degree bend. Except
        // when we're on a pin, because pins always need bends, and junctions are just added to
        // pins for visual clarity.
        else if( !foundLine && foundJunction && !foundPin )
        {
            // Create a new wire ending at the unselected end
            foundLine = new SCH_LINE( unselectedEnd, line->GetLayer() );
            foundLine->SetFlags( IS_NEW );
            foundLine->SetLastResolvedState( line );
            m_frame->AddToScreen( foundLine, m_frame->GetScreen() );
            m_newDragLines.insert( foundLine );

            // We just broke off of the existing items, so replace all of them with our new
            // end connection.
            m_lineConnectionCache[foundLine] = m_lineConnectionCache[line];
            m_lineConnectionCache[line].clear();
            m_lineConnectionCache[line].emplace_back( foundLine );
        }

        // We want to drag our found line if it's in the same angle as the move or zero length,
        // but if the original drag line is also zero and the same original angle we should extend
        // that one first
        if( foundLine && !preferOriginalLine )
        {
            // Move the connected line found oriented in the direction of our move.
            //
            // Make sure we grab the right endpoint, it's not always STARTPOINT since the user can
            // draw a box of lines. We need to only move one though, and preferably the start point,
            // in case we have a zero length line that we are extending (we want the foundLine
            // start point to be attached to the unselected end of our drag line).
            //
            // Also, new lines are added already so they'll be in the undo list, skip adding them.

            if( !foundLine->HasFlag( IS_CHANGED ) && !foundLine->HasFlag( IS_NEW ) )
            {
                aCommit->Modify( (SCH_ITEM*) foundLine, m_frame->GetScreen() );

                if( !foundLine->IsSelected() )
                    m_changedDragLines.insert( foundLine );
            }

            if( foundLine->GetStartPoint() == unselectedEnd )
                foundLine->MoveStart( splitDelta );
            else if( foundLine->GetEndPoint() == unselectedEnd )
                foundLine->MoveEnd( splitDelta );

            updateItem( foundLine, true );

            SCH_LINE* bendLine = nullptr;

            if( m_lineConnectionCache.count( foundLine ) == 1
                    && m_lineConnectionCache[foundLine][0]->Type() == SCH_LINE_T )
            {
                bendLine = static_cast<SCH_LINE*>( m_lineConnectionCache[foundLine][0] );
            }

            // Remerge segments we've created if this is a segment that we've added whose only
            // other connection is also an added segment
            //
            // bendLine is first added segment at the original attachment point, foundLine is the
            // orthogonal line between bendLine and this line
            if( foundLine->HasFlag( IS_NEW )
                    && foundLine->GetLength() == 0
                    && bendLine && bendLine->HasFlag( IS_NEW ) )
            {
                if( line->HasFlag( STARTPOINT ) )
                    line->SetEndPoint( bendLine->GetEndPoint() );
                else
                    line->SetStartPoint( bendLine->GetEndPoint() );

                // Update our cache of the connected items.

                // First, re-attach our drag labels to the original line being re-merged.
                for( EDA_ITEM* candidate : m_lineConnectionCache[bendLine] )
                {
                    SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( candidate );

                    if( label && m_specialCaseLabels.count( label ) )
                        m_specialCaseLabels[label].attachedLine = line;
                }

                m_lineConnectionCache[line] = m_lineConnectionCache[bendLine];
                m_lineConnectionCache[bendLine].clear();
                m_lineConnectionCache[foundLine].clear();

                m_frame->RemoveFromScreen( bendLine, m_frame->GetScreen() );
                m_frame->RemoveFromScreen( foundLine, m_frame->GetScreen() );

                m_newDragLines.erase( bendLine );
                m_newDragLines.erase( foundLine );

                delete bendLine;
                delete foundLine;
            }
            //Ok, move the unselected end of our item
            else
            {
                if( line->HasFlag( STARTPOINT ) )
                    line->MoveEnd( splitDelta );
                else
                    line->MoveStart( splitDelta );
            }

            updateItem( line, true );
        }
        else if( line->GetLength() == 0 )
        {
            // We didn't find another line to shorten/lengthen, (or we did but it's also zero)
            // so now is a good time to use our existing zero-length original line
        }
        // Either no line was at the "right" angle, or this was a junction, pin, sheet, etc. We
        // need to add segments to keep the soon-to-move unselected end connected to these items.
        //
        // To keep our drag selections all the same, we'll move our unselected end point and then
        // put wires between it and its original endpoint.
        else if( foundAttachment && line->IsOrthogonal() )
        {
            VECTOR2D lineGrid = grid.GetGridSize( grid.GetItemGrid( line ) );

            // The bend counter handles a group of wires all needing their offset one grid movement
            // further out from each other to not overlap.  The absolute value stuff finds the
            // direction of the line and hence the the bend increment on that axis
            unsigned int xMoveBit = splitDelta.x != 0;
            unsigned int yMoveBit = splitDelta.y != 0;
            int          xLength = abs( unselectedEnd.x - selectedEnd.x );
            int          yLength = abs( unselectedEnd.y - selectedEnd.y );
            int          xMove = ( xLength - ( xBendCount * lineGrid.x ) )
                                    * sign( selectedEnd.x - unselectedEnd.x );
            int          yMove = ( yLength - ( yBendCount * lineGrid.y ) )
                                    * sign( selectedEnd.y - unselectedEnd.y );

            // Create a new wire ending at the unselected end, we'll move the new wire's start
            // point to the unselected end
            SCH_LINE* a = new SCH_LINE( unselectedEnd, line->GetLayer() );
            a->MoveStart( VECTOR2I( xMove, yMove ) );
            a->SetFlags( IS_NEW );
            a->SetConnectivityDirty( true );
            a->SetLastResolvedState( line );
            m_frame->AddToScreen( a, m_frame->GetScreen() );
            m_newDragLines.insert( a );

            SCH_LINE* b = new SCH_LINE( a->GetStartPoint(), line->GetLayer() );
            b->MoveStart( VECTOR2I( splitDelta.x, splitDelta.y ) );
            b->SetFlags( IS_NEW | STARTPOINT );
            b->SetConnectivityDirty( true );
            b->SetLastResolvedState( line );
            m_frame->AddToScreen( b, m_frame->GetScreen() );
            m_newDragLines.insert( b );

            xBendCount += yMoveBit;
            yBendCount += xMoveBit;

            // Ok move the unselected end of our item
            if( line->HasFlag( STARTPOINT ) )
            {
                line->MoveEnd( VECTOR2I( splitDelta.x ? splitDelta.x : xMove,
                                         splitDelta.y ? splitDelta.y : yMove ) );
            }
            else
            {
                line->MoveStart( VECTOR2I( splitDelta.x ? splitDelta.x : xMove,
                                           splitDelta.y ? splitDelta.y : yMove ) );
            }

            // Update our cache of the connected items. First, attach our drag labels to the line
            // left behind.
            for( EDA_ITEM* candidate : m_lineConnectionCache[line] )
            {
                SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( candidate );

                if( label && m_specialCaseLabels.count( label ) )
                    m_specialCaseLabels[label].attachedLine = a;
            }

            // We just broke off of the existing items, so replace all of them with our new end
            // connection.
            m_lineConnectionCache[a] = m_lineConnectionCache[line];
            m_lineConnectionCache[b].emplace_back( a );
            m_lineConnectionCache[line].clear();
            m_lineConnectionCache[line].emplace_back( b );
        }
        // Original line has no attachments, just move the unselected end
        else if( !foundAttachment )
        {
            if( line->HasFlag( STARTPOINT ) )
                line->MoveEnd( splitDelta );
            else
                line->MoveStart( splitDelta );
        }
    }
}


int SCH_MOVE_TOOL::Main( const TOOL_EVENT& aEvent )
{
    m_isDrag = aEvent.IsAction( &SCH_ACTIONS::drag );

    if( SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() ) )
    {
        bool isSlice = false;

        if( m_isDrag )
            isSlice = aEvent.Parameter<bool>();

        wxCHECK( aEvent.SynchronousState(), 0 );
        aEvent.SynchronousState()->store( STS_RUNNING );

        if( doMoveSelection( aEvent, commit, isSlice ) )
            aEvent.SynchronousState()->store( STS_FINISHED );
        else
            aEvent.SynchronousState()->store( STS_CANCELLED );
    }
    else
    {
        SCH_COMMIT localCommit( m_toolMgr );

        if( doMoveSelection( aEvent, &localCommit, false ) )
            localCommit.Push( m_isDrag ? _( "Drag" ) : _( "Move" ) );
        else
            localCommit.Revert();
    }

    return 0;
}


bool SCH_MOVE_TOOL::doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit, bool aIsSlice )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    bool                  wasDragging = m_moveInProgress && m_isDrag;
    bool                  isLineModeConstrained = false;

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        isLineModeConstrained = cfg->m_Drawing.line_mode != LINE_MODE::LINE_MODE_FREE;

    m_anchorPos.reset();

    if( m_moveInProgress )
    {
        if( m_isDrag != wasDragging )
        {
            EDA_ITEM* sel = m_selectionTool->GetSelection().Front();

            if( sel && !sel->IsNew() )
            {
                // Reset the selected items so we can start again with the current m_isDrag
                // state.
                aCommit->Revert();

                m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
                m_anchorPos = m_cursor - m_moveOffset;
                m_moveInProgress = false;
                controls->SetAutoPan( false );

                // And give it a kick so it doesn't have to wait for the first mouse movement
                // to refresh.
                m_toolMgr->PostAction( SCH_ACTIONS::restartMove );
            }
        }
        else
        {
            // The tool hotkey is interpreted as a click when already dragging/moving
            m_toolMgr->PostAction( ACTIONS::cursorClick );
        }

        return false;
    }

    if( m_inMoveTool )      // Must come after m_moveInProgress checks above...
        return false;

    REENTRANCY_GUARD guard( &m_inMoveTool );

    SCH_SELECTION& userSelection = m_selectionTool->GetSelection();

    // If a single pin is selected, promote the move selection to its parent symbol
    if( userSelection.GetSize() == 1 )
    {
        EDA_ITEM* selItem = userSelection.Front();

        if( selItem->Type() == SCH_PIN_T )
        {
            EDA_ITEM* parent = selItem->GetParent();

            if( parent->Type() == SCH_SYMBOL_T )
            {
                m_selectionTool->ClearSelection();
                m_selectionTool->AddItemToSel( parent );
            }
        }
    }

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems,
                                                                  true );
    bool           unselect = selection.IsHover();

    // Keep an original copy of the starting points for cleanup after the move
    std::vector<DANGLING_END_ITEM> internalPoints;

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );

    m_frame->PushTool( aEvent );

    if( selection.Empty() )
    {
        // Note that it's important to go through push/pop even when the selection is empty.
        // This keeps other tools from having to special-case an empty move.
        m_frame->PopTool( aEvent );
        return false;
    }

    bool        restore_state = false;
    TOOL_EVENT  copy = aEvent;
    TOOL_EVENT* evt = &copy;
    VECTOR2I    prevPos;
    GRID_HELPER_GRIDS snapLayer = GRID_CURRENT;

    m_cursor = controls->GetCursorPosition();

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( evt->IsAction( &SCH_ACTIONS::restartMove )
                || evt->IsAction( &SCH_ACTIONS::move )
                || evt->IsAction( &SCH_ACTIONS::drag )
                || evt->IsMotion()
                || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                SCH_ITEM* sch_item = (SCH_ITEM*) selection.Front();
                bool      placingNewItems = sch_item && sch_item->IsNew();

                //------------------------------------------------------------------------
                // Setup a drag or a move
                //
                m_dragAdditions.clear();
                m_specialCaseLabels.clear();
                m_specialCaseSheetPins.clear();
                internalPoints.clear();
                clearNewDragLines();

                for( SCH_ITEM* it : m_frame->GetScreen()->Items() )
                {
                    it->ClearFlags( SELECTED_BY_DRAG );

                    if( !it->IsSelected() )
                        it->ClearFlags( STARTPOINT | ENDPOINT );
                }

                // Drag of split items start over top of their other segment so
                // we want to skip grabbing the segments we split from
                if( m_isDrag && !aIsSlice )
                {
                    EDA_ITEMS connectedDragItems;

                    // Add connections to the selection for a drag.
                    // Do all non-labels/entries first so we don't add junctions to drag
                    // when the line will eventually be drag selected.
                    std::vector<SCH_ITEM*> stageTwo;

                    for( EDA_ITEM* edaItem : selection )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
                        std::vector<VECTOR2I> connections;

                        switch( item->Type() )
                        {
                        case SCH_LABEL_T:
                        case SCH_HIER_LABEL_T:
                        case SCH_GLOBAL_LABEL_T:
                        case SCH_DIRECTIVE_LABEL_T:
                            stageTwo.emplace_back(item);
                            break;

                        case SCH_LINE_T:
                            static_cast<SCH_LINE*>( item )->GetSelectedPoints( connections );
                            break;
                        default:
                            connections = item->GetConnectionPoints();
                        }

                        for( const VECTOR2I& point : connections )
                            getConnectedDragItems( aCommit, item, point, connectedDragItems );
                    }

                    // Go back and get all label connections now that we can test for drag-selected
                    // lines the labels might be on
                    for( SCH_ITEM* item : stageTwo )
                    {
                        for( const VECTOR2I& point : item->GetConnectionPoints() )
                            getConnectedDragItems( aCommit, item, point, connectedDragItems );
                    }

                    for( EDA_ITEM* item : connectedDragItems )
                    {
                        m_dragAdditions.push_back( item->m_Uuid );
                        m_selectionTool->AddItemToSel( item, QUIET_MODE );
                    }

                    // Pre-cache all connections of our selected objects so we can keep track of
                    // what they were originally connected to as we drag them around
                    for( EDA_ITEM* edaItem : selection )
                    {
                        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( edaItem );

                        if( schItem->Type() == SCH_LINE_T )
                        {
                            SCH_LINE* line = static_cast<SCH_LINE*>( schItem );

                            //Also store the original angle of the line, is needed later to decide
                            //which segment to extend when they've become zero length
                            line->StoreAngle();

                            for( const VECTOR2I& point : line->GetConnectionPoints() )
                                getConnectedItems( line, point, m_lineConnectionCache[line] );
                        }
                    }
                }
                else
                {
                    // Mark the edges of the block with dangling flags for a move.
                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->GetEndPoints( internalPoints );

                    std::vector<DANGLING_END_ITEM> endPointsByType = internalPoints;
                    std::vector<DANGLING_END_ITEM> endPointsByPos = endPointsByType;
                    DANGLING_END_ITEM_HELPER::sort_dangling_end_items( endPointsByType, endPointsByPos );

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( endPointsByType, endPointsByPos );
                }

                // Hide junctions connected to line endpoints that are not selected
                m_hiddenJunctions.clear();

                for( EDA_ITEM* edaItem : selection )
                {
                    if( edaItem->Type() != SCH_LINE_T )
                        continue;

                    SCH_LINE* line = static_cast<SCH_LINE*>( edaItem );

                    for( const VECTOR2I& pt : line->GetConnectionPoints() )
                    {
                        SCH_JUNCTION* jct = static_cast<SCH_JUNCTION*>( m_frame->GetScreen()->GetItem( pt, 0, SCH_JUNCTION_T ) );

                        if( jct && !jct->IsSelected()
                            && std::find( m_hiddenJunctions.begin(), m_hiddenJunctions.end(), jct ) == m_hiddenJunctions.end() )
                        {
                            m_view->Hide( jct, true );
                            m_hiddenJunctions.push_back( jct );
                        }
                    }
                }

                // Generic setup
                snapLayer = grid.GetSelectionGrid( selection );

                for( EDA_ITEM* item : selection )
                {
                    SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

                    if( schItem->IsNew() )
                    {
                        // Item was added to commit in a previous command

                        // While SCH_COMMIT::Push() will add any new items to the entered group,
                        // we need to do it earlier so that the previews while moving are correct.
                        if( SCH_GROUP* enteredGroup = m_selectionTool->GetEnteredGroup() )
                        {
                            if( schItem->IsGroupableType() && !schItem->GetParentGroup() )
                            {
                                aCommit->Modify( enteredGroup, m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );
                                enteredGroup->AddItem( schItem );
                            }
                        }
                    }
                    else if( schItem->GetParent() && schItem->GetParent()->IsSelected() )
                    {
                        // Item will be (or has been) added to commit by parent
                    }
                    else
                    {
                        aCommit->Modify( schItem, m_frame->GetScreen(), RECURSE_MODE::RECURSE );
                    }

                    schItem->SetFlags( IS_MOVING );

                    if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( schItem ) )
                    {
                        shape->SetHatchingDirty();
                        shape->UpdateHatching();
                    }

                    schItem->RunOnChildren(
                            [&]( SCH_ITEM* schItem )
                            {
                                item->SetFlags( IS_MOVING );
                            },
                            RECURSE_MODE::RECURSE );

                    schItem->SetStoredPos( schItem->GetPosition() );
                }

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( evt->IsAction( &SCH_ACTIONS::restartMove ) )
                {
                    wxASSERT_MSG( m_anchorPos, "Should be already set from previous cmd" );
                }
                else if( placingNewItems )
                {
                    m_anchorPos = selection.GetReferencePoint();
                }

                if( m_anchorPos )
                {
                    VECTOR2I delta = m_cursor - (*m_anchorPos);
                    bool     isPasted = false;

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta );
                        updateItem( item, false );

                        isPasted |= ( item->GetFlags() & IS_PASTED ) != 0;
                        item->ClearFlags( IS_PASTED );
                    }

                    // The first time pasted items are moved we need to store the position of the
                    // cursor so that rotate while moving works as expected (instead of around the
                    // original anchor point
                    if( isPasted )
                        selection.SetReferencePoint( m_cursor );

                    m_anchorPos = m_cursor;
                }
                // For some items, moving the cursor to anchor is not good (for instance large
                // hierarchical sheets or symbols can have the anchor outside the view)
                else if( selection.Size() == 1 && !sch_item->IsMovableFromAnchorPoint() )
                {
                    m_cursor = getViewControls()->GetCursorPosition( true );
                    m_anchorPos = m_cursor;
                }
                else
                {
                    if( m_frame->GetMoveWarpsCursor() )
                    {
                        // User wants to warp the mouse
                        m_cursor = grid.BestDragOrigin( m_cursor, snapLayer, selection );
                        selection.SetReferencePoint( m_cursor );
                    }
                    else
                    {
                        // User does not want to warp the mouse
                        m_cursor = getViewControls()->GetCursorPosition( true );
                    }
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_view->ClearPreview();

            m_cursor = grid.BestSnapAnchor( controls->GetCursorPosition( false ),
                                        snapLayer, selection );

            VECTOR2I delta( m_cursor - prevPos );
            m_anchorPos = m_cursor;

            // We need to check if the movement will change the net offset direction on the
            // X an Y axes. This is because we remerge added bend lines in realtime, and we
            // also account for the direction of the move when adding bend lines. So, if the
            // move direction changes, we need to split it into a move that gets us back to
            // zero, then the rest of the move.
            std::vector<VECTOR2I> splitMoves;

            if( alg::signbit( m_moveOffset.x ) != alg::signbit( ( m_moveOffset + delta ).x ) )
            {
                splitMoves.emplace_back( VECTOR2I( -1 * m_moveOffset.x, 0 ) );
                splitMoves.emplace_back( VECTOR2I( delta.x + m_moveOffset.x, 0 ) );
            }
            else
            {
                splitMoves.emplace_back( VECTOR2I( delta.x, 0 ) );
            }

            if( alg::signbit( m_moveOffset.y ) != alg::signbit( ( m_moveOffset + delta ).y ) )
            {
                splitMoves.emplace_back( VECTOR2I( 0, -1 * m_moveOffset.y ) );
                splitMoves.emplace_back( VECTOR2I( 0, delta.y + m_moveOffset.y ) );
            }
            else
            {
                splitMoves.emplace_back( VECTOR2I( 0, delta.y ) );
            }


            m_moveOffset += delta;
            prevPos = m_cursor;

            // Used for tracking how far off a drag end should have its 90 degree elbow added
            int xBendCount = 1;
            int yBendCount = 1;

            // Split the move into X and Y moves so we can correctly drag orthogonal lines
            for( const VECTOR2I& splitDelta : splitMoves )
            {
                // Skip non-moves
                if( splitDelta == VECTOR2I( 0, 0 ) )
                    continue;

                for( EDA_ITEM* item : selection.GetItemsSortedByTypeAndXY( ( delta.x >= 0 ),
                                                                           ( delta.y >= 0 ) ) )
                {
                    // Don't double move pins, fields, etc.
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );

                    // Only partially selected drag lines in orthogonal line mode need special
                    // handling
                    if( m_isDrag && isLineModeConstrained
                            && line && line->HasFlag( STARTPOINT ) != line->HasFlag( ENDPOINT ) )
                    {
                        orthoLineDrag( aCommit, line, splitDelta, xBendCount, yBendCount, grid );
                    }

                    // Move all other items normally, including the selected end of partially
                    // selected lines
                    moveItem( item, splitDelta );
                    updateItem( item, false );

                    // Update any lines connected to sheet pins to the sheet pin's location
                    // (which may not exactly follow the splitDelta as the pins are constrained
                    // along the sheet edges.
                    for( const auto& [pin, lineEnd] : m_specialCaseSheetPins )
                    {
                        if( lineEnd.second && lineEnd.first->HasFlag( STARTPOINT ) )
                            lineEnd.first->SetStartPoint( pin->GetPosition() );
                        else if( !lineEnd.second && lineEnd.first->HasFlag( ENDPOINT ) )
                            lineEnd.first->SetEndPoint( pin->GetPosition() );
                    }
                }
            }

            if( selection.HasReferencePoint() )
                selection.SetReferencePoint( selection.GetReferencePoint() + delta );

            std::vector<SCH_ITEM*> previewItems;

            for( EDA_ITEM* it : selection )
                previewItems.push_back( static_cast<SCH_ITEM*>( it ) );

            for( SCH_LINE* line : m_newDragLines )
                previewItems.push_back( line );

            for( SCH_LINE* line : m_changedDragLines )
                previewItems.push_back( line );

            for( SCH_JUNCTION* jct : JUNCTION_HELPERS::PreviewJunctions( m_frame->GetScreen(),
                                                                          previewItems ) )
            {
                m_view->AddToPreview( jct, true );
            }

            m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
        }

        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive()
                 || evt->IsActivate()
                 || evt->IsAction( &ACTIONS::undo ) )
        {
            if( evt->IsCancelInteractive() )
                m_frame->GetInfoBar()->Dismiss();

            if( m_moveInProgress )
            {
                if( evt->IsActivate() )
                {
                    // Allowing other tools to activate during a move runs the risk of race
                    // conditions in which we try to spool up both event loops at once.

                    if( m_isDrag )
                        m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel drag." ) );
                    else
                        m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel move." ) );

                    evt->SetPassEvent( false );
                    continue;
                }

                evt->SetPassEvent( false );
                restore_state = true;
            }

            clearNewDragLines();

            m_view->ClearPreview();

            break;
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->IsAction( &ACTIONS::doDelete ) )
        {
            evt->SetPassEvent();
            // Exit on a delete; there will no longer be anything to drag.
            break;
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                 || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                 || evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else if( evt->IsAction( &SCH_ACTIONS::rotateCW ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::rotateCW, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::rotateCCW ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::rotateCCW, aCommit );
        }
        else if( evt->IsAction( &ACTIONS::increment ) )
        {
            m_toolMgr->RunSynchronousAction( ACTIONS::increment, aCommit, evt->Parameter<ACTIONS::INCREMENT>() );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toCLabel ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toCLabel, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toGLabel ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toGLabel, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toHLabel ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toHLabel, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toLabel ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toLabel, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toText ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toText, aCommit );
        }
        else if( evt->IsAction( &SCH_ACTIONS::toTextBox ) )
        {
            m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toTextBox, aCommit );
        }
        else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT
                && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_END )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT;

                if( symbol )
                {
                    m_frame->SelectUnit( symbol, unit );
                    m_toolMgr->PostAction( ACTIONS::refreshPreview );
                }
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_BASE
                     && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_ALT )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );
                int bodyStyle = ( *evt->GetCommandId() - ID_POPUP_SCH_SELECT_BASE ) + 1;

                if( symbol && symbol->GetBodyStyle() != bodyStyle )
                {
                    m_frame->FlipBodyStyle( symbol );
                    m_toolMgr->PostAction( ACTIONS::refreshPreview );
                }
            }
        }
        else if( evt->IsAction( &SCH_ACTIONS::highlightNet )
                    || evt->IsAction( &SCH_ACTIONS::selectOnPCB ) )
        {
            // These don't make any sense during a move.  Eat them.
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT )
                || evt->IsClick( BUT_LEFT )
                || evt->IsDblClick( BUT_LEFT ) )
        {
            break; // Finish
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( m_moveInProgress );

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    // Create a selection of original selection, drag selected/changed items, and new
    // bend lines for later before we clear them in the aCommit. We'll need these
    // to check for new junctions needed, etc.
    SCH_SELECTION selectionCopy( selection );

    for( SCH_LINE* line : m_newDragLines )
        selectionCopy.Add( line );

    for( SCH_LINE* line : m_changedDragLines )
        selectionCopy.Add( line );

    // Save whatever new bend lines and changed lines survived the drag
    for( SCH_LINE* newLine : m_newDragLines )
    {
        newLine->ClearEditFlags();
        aCommit->Added( newLine, m_frame->GetScreen() );
    }

    // These lines have been changed, but aren't selected. We need
    // to manually clear these edit flags or they'll stick around.
    for( SCH_LINE* oldLine : m_changedDragLines )
        oldLine->ClearEditFlags();

    m_newDragLines.clear();
    m_changedDragLines.clear();

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_moveOffset = { 0, 0 };
    m_anchorPos.reset();

    if( restore_state )
    {
        m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
    }
    else
    {
        // One last update after exiting loop (for slower stuff, such as updating SCREEN's RTree).
        for( EDA_ITEM* item : selection )
        {
            updateItem( item, true );

            if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item ) )
                sch_item->SetConnectivityDirty( true );
        }

        if( selection.GetSize() == 1 && selection.Front()->IsNew() )
            m_frame->SaveCopyForRepeatItem( static_cast<SCH_ITEM*>( selection.Front() ) );

        m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );

        // If we move items away from a junction, we _may_ want to add a junction there
        // to denote the state.
        for( const DANGLING_END_ITEM& it : internalPoints )
        {
            if( m_frame->GetScreen()->IsExplicitJunctionNeeded( it.GetPosition()) )
                m_frame->AddJunction( aCommit, m_frame->GetScreen(), it.GetPosition() );
        }

        SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
        lwbTool->TrimOverLappingWires( aCommit, &selectionCopy );
        lwbTool->AddJunctionsIfNeeded( aCommit, &selectionCopy );

        // This needs to run prior to `RecalculateConnections` because we need to identify
        // the lines that are newly dangling
        if( m_isDrag && !aIsSlice )
            trimDanglingLines( aCommit );

        // Auto-rotate any moved labels
        for( EDA_ITEM* item : selection )
            m_frame->AutoRotateItem( m_frame->GetScreen(), static_cast<SCH_ITEM*>( item ) );

        m_frame->Schematic().CleanUp( aCommit );
    }

    for( EDA_ITEM* item : m_frame->GetScreen()->Items() )
        item->ClearEditFlags();

    // ensure any selected item not in screen main list (for instance symbol fields)
    // has its edit flags cleared
    for( EDA_ITEM* item : selectionCopy )
        item->ClearEditFlags();

    if( unselect )
        m_toolMgr->RunAction( ACTIONS::selectionClear );
    else
        m_selectionTool->RebuildSelection();  // Schematic cleanup might have merged lines, etc.

    m_dragAdditions.clear();
    m_lineConnectionCache.clear();
    m_moveInProgress = false;

    for( SCH_JUNCTION* jct : m_hiddenJunctions )
        m_view->Hide( jct, false );

    m_hiddenJunctions.clear();
    m_view->ClearPreview();
    m_frame->PopTool( aEvent );

    return !restore_state;
}


void SCH_MOVE_TOOL::trimDanglingLines( SCH_COMMIT* aCommit )
{
    // Need a local cleanup first to ensure we remove unneeded junctions
    m_frame->Schematic().CleanUp( aCommit, m_frame->GetScreen() );

    std::set<SCH_ITEM*> danglers;

    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                m_toolMgr->GetView()->Update( aChangedItem, KIGFX::REPAINT );

                // Delete newly dangling lines:
                // Find split segments (one segment is new, the other is changed) that
                // we aren't dragging and don't have selected
                if( aChangedItem->HasFlag( IS_BROKEN) && aChangedItem->IsDangling()
                  && !aChangedItem->IsSelected() )
                {
                    danglers.insert( aChangedItem );
                }
            };

    m_frame->GetScreen()->TestDanglingEnds( nullptr, &changeHandler );

    for( SCH_ITEM* line : danglers )
    {
        line->SetFlags( STRUCT_DELETED );
        aCommit->Removed( line, m_frame->GetScreen() );

        updateItem( line, false );
        m_frame->RemoveFromScreen( line, m_frame->GetScreen() );
    }
}


void SCH_MOVE_TOOL::getConnectedItems( SCH_ITEM* aOriginalItem, const VECTOR2I& aPoint,
                                       EDA_ITEMS& aList )
{
    EE_RTREE&         items = m_frame->GetScreen()->Items();
    EE_RTREE::EE_TYPE itemsOverlapping = items.Overlapping( aOriginalItem->GetBoundingBox() );
    SCH_ITEM*         foundJunction = nullptr;
    SCH_ITEM*         foundSymbol   = nullptr;

    // If you're connected to a junction, you're only connected to the junction.
    //
    // But, if you're connected to a junction on a pin, you're only connected to the pin. This
    // is because junctions and pins have different logic for how bend lines are generated and
    // we need to prioritize the pin version in some cases.
    for( SCH_ITEM* item : itemsOverlapping )
    {
        if( item != aOriginalItem && item->IsConnected( aPoint ) )
        {
            if( item->Type() == SCH_JUNCTION_T )
                foundJunction = item;
            else if( item->Type() == SCH_SYMBOL_T )
                foundSymbol = item;
        }
    }

    if( foundSymbol && foundJunction )
    {
        aList.push_back( foundSymbol );
        return;
    }

    if( foundJunction )
    {
        aList.push_back( foundJunction );
        return;
    }


    for( SCH_ITEM* test : itemsOverlapping )
    {
        if( test == aOriginalItem || !test->CanConnect( aOriginalItem ) )
            continue;

        switch( test->Type() )
        {
        case SCH_LINE_T:
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( test );

            // When getting lines for the connection cache, it's important that we only add
            // items at the unselected end, since that is the only end that is handled specially.
            // Fully selected lines, and the selected end of a partially selected line, are moved
            // around normally and don't care about their connections.
            if( ( line->HasFlag( STARTPOINT ) && aPoint == line->GetStartPoint() )
                || ( line->HasFlag( ENDPOINT ) && aPoint == line->GetEndPoint() ) )
            {
                continue;
            }

            if( test->IsConnected( aPoint ) )
                aList.push_back( test );

            // Labels can connect to a wire (or bus) anywhere along the length
            if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aOriginalItem ) )
            {
                if( static_cast<SCH_LINE*>( test )->HitTest( label->GetPosition(), 1 ) )
                    aList.push_back( test );
            }

            break;
        }

        case SCH_SHEET_T:
            if( aOriginalItem->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( aOriginalItem );

                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( test )->GetPins() )
                {
                    if( pin->IsConnected( aPoint ) )
                    {
                        if( pin->IsSelected() )
                            m_specialCaseSheetPins[pin] = { line, line->GetStartPoint() == aPoint };

                        aList.push_back( pin );
                    }
                }
            }

            break;

        case SCH_SYMBOL_T:
        case SCH_JUNCTION_T:
        case SCH_NO_CONNECT_T:
            if( test->IsConnected( aPoint ) )
                aList.push_back( test );

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            // Labels can connect to a wire (or bus) anywhere along the length
            if( aOriginalItem->Type() == SCH_LINE_T && test->CanConnect( aOriginalItem ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( test );
                SCH_LINE*       line = static_cast<SCH_LINE*>( aOriginalItem );

                if( line->HitTest( label->GetPosition(), 1 ) )
                    aList.push_back( label );
            }

            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            if( aOriginalItem->Type() == SCH_LINE_T && test->CanConnect( aOriginalItem ) )
            {
                SCH_TEXT* label = static_cast<SCH_TEXT*>( test );
                SCH_LINE* line = static_cast<SCH_LINE*>( aOriginalItem );

                if( line->HitTest( aPoint, 1 ) )
                    aList.push_back( label );
            }

            break;

        default:
            break;
        }
    }
}


void SCH_MOVE_TOOL::getConnectedDragItems( SCH_COMMIT* aCommit, SCH_ITEM* aSelectedItem,
                                           const VECTOR2I& aPoint, EDA_ITEMS& aList )
{
    EE_RTREE&         items = m_frame->GetScreen()->Items();
    EE_RTREE::EE_TYPE itemsOverlappingRTree = items.Overlapping( aSelectedItem->GetBoundingBox() );
    std::vector<SCH_ITEM*> itemsConnectable;
    bool              ptHasUnselectedJunction = false;

    auto makeNewWire =
            [this]( SCH_COMMIT* commit, SCH_ITEM* fixed, SCH_ITEM* selected, const VECTOR2I& start,
                    const VECTOR2I& end )
            {
                SCH_LINE* newWire;

                // Add a new newWire between the fixed item and the selected item so the selected
                // item can be dragged.
                if( fixed->GetLayer() == LAYER_BUS_JUNCTION || fixed->GetLayer() == LAYER_BUS
                    || selected->GetLayer() == LAYER_BUS )
                {
                    newWire = new SCH_LINE( start, LAYER_BUS );
                }
                else
                {
                    newWire = new SCH_LINE( start, LAYER_WIRE );
                }

                newWire->SetFlags( IS_NEW );
                newWire->SetConnectivityDirty( true );

                if( dynamic_cast<const SCH_LINE*>( selected ) )
                    newWire->SetLastResolvedState( selected );
                else if( dynamic_cast<const SCH_LINE*>( fixed ) )
                    newWire->SetLastResolvedState( fixed );

                newWire->SetEndPoint( end );
                m_frame->AddToScreen( newWire, m_frame->GetScreen() );
                commit->Added( newWire, m_frame->GetScreen() );

                return newWire;
            };

    auto makeNewJunction =
            [this]( SCH_COMMIT* commit, SCH_LINE* line, const VECTOR2I& pt )
            {
                SCH_JUNCTION* junction = new SCH_JUNCTION( pt );
                junction->SetFlags( IS_NEW );
                junction->SetConnectivityDirty( true );
                junction->SetLastResolvedState( line );

                if( line->IsBus() )
                    junction->SetLayer( LAYER_BUS_JUNCTION );

                m_frame->AddToScreen( junction, m_frame->GetScreen() );
                commit->Added( junction, m_frame->GetScreen() );

                return junction;
            };

    for( SCH_ITEM* item : itemsOverlappingRTree )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                if( !pin->IsSelected()
                    && pin->GetPosition() == aSelectedItem->GetPosition()
                    && pin->CanConnect( aSelectedItem ) )
                {
                    itemsConnectable.push_back( pin );
                }
            }

            continue;
        }

        // Skip ourselves, skip already selected items (but not lines, they need both ends tested)
        // and skip unconnectable items
        if( item == aSelectedItem
            || ( item->Type() != SCH_LINE_T && item->IsSelected() )
            || !item->CanConnect( aSelectedItem ) )
        {
            continue;
        }

        itemsConnectable.push_back( item );
    }

    for( SCH_ITEM* item : itemsConnectable )
    {
        if( item->Type() == SCH_JUNCTION_T && item->IsConnected( aPoint ) && !item->IsSelected() )
        {
            ptHasUnselectedJunction = true;
            break;
        }
    }

    SCH_LINE* newWire = nullptr;

    for( SCH_ITEM* test : itemsConnectable )
    {
        KICAD_T testType = test->Type();

        switch( testType )
        {
        case SCH_LINE_T:
        {
            // Select the connected end of wires/bus connections that don't have an unselected
            // junction isolating them from the drag
            if( ptHasUnselectedJunction )
                break;

            SCH_LINE* line = static_cast<SCH_LINE*>( test );

            if( line->GetStartPoint() == aPoint )
            {
                // It's possible to manually select one end of a line and get a drag
                // connected other end, so we set the flag and then early exit the loop
                // later if the other drag items like labels attached to the line have
                // already been grabbed during the partial selection process.
                line->SetFlags( STARTPOINT );

                if( line->HasFlag( SELECTED ) || line->HasFlag( SELECTED_BY_DRAG ) )
                {
                    continue;
                }
                else
                {
                    line->SetFlags( SELECTED_BY_DRAG );
                    aList.push_back( line );
                }
            }
            else if( line->GetEndPoint() == aPoint )
            {
                line->SetFlags( ENDPOINT );

                if( line->HasFlag( SELECTED ) || line->HasFlag( SELECTED_BY_DRAG ) )
                {
                    continue;
                }
                else
                {
                    line->SetFlags( SELECTED_BY_DRAG );
                    aList.push_back( line );
                }
            }
            else
            {
                switch( aSelectedItem->Type() )
                {
                // These items can connect anywhere along a line
                case SCH_BUS_BUS_ENTRY_T:
                case SCH_BUS_WIRE_ENTRY_T:
                case SCH_LABEL_T:
                case SCH_HIER_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_DIRECTIVE_LABEL_T:
                    // Only add a line if this line is unselected; if the label and line are both
                    // selected they'll move together
                    if( line->HitTest( aPoint, 1 ) && !line->HasFlag( SELECTED )
                        && !line->HasFlag( SELECTED_BY_DRAG ) )
                    {
                        newWire = makeNewWire( aCommit, line, aSelectedItem, aPoint, aPoint );
                        newWire->SetFlags( SELECTED_BY_DRAG | STARTPOINT );
                        newWire->StoreAngle( ( line->Angle() + ANGLE_90 ).Normalize() );
                        aList.push_back( newWire );

                        if( aPoint != line->GetStartPoint() && aPoint != line->GetEndPoint() )
                        {
                            // Split line in half
                            if( !line->IsNew() )
                                aCommit->Modify( line, m_frame->GetScreen() );

                            VECTOR2I oldEnd = line->GetEndPoint();
                            line->SetEndPoint( aPoint );

                            makeNewWire( aCommit, line, line, aPoint, oldEnd );
                            makeNewJunction( aCommit, line, aPoint );
                        }
                        else
                        {
                            m_lineConnectionCache[ newWire ] = { line };
                            m_lineConnectionCache[ line ] = { newWire };
                        }
                    }
                    break;

                default:
                    break;
                }

                break;
            }

            // Since only one end is going to move, the movement vector of any labels attached to
            // it is scaled by the proportion of the line length the label is from the moving end.
            for( SCH_ITEM* item : items.Overlapping( line->GetBoundingBox() ) )
            {
                SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item );

                if( !label || label->IsSelected() )
                    continue;   // These will be moved on their own because they're selected

                if( label->HasFlag( SELECTED_BY_DRAG ) )
                    continue;

                if( label->CanConnect( line ) && line->HitTest( label->GetPosition(), 1 ) )
                {
                    label->SetFlags( SELECTED_BY_DRAG );
                    aList.push_back( label );

                    SPECIAL_CASE_LABEL_INFO info;
                    info.attachedLine = line;
                    info.originalLabelPos = label->GetPosition();
                    m_specialCaseLabels[label] = info;
                }
            }

            break;
        }

        case SCH_SHEET_T:
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( test )->GetPins() )
            {
                if( pin->IsConnected( aPoint ) )
                {
                    if( pin->IsSelected() && aSelectedItem->Type() == SCH_LINE_T )
                    {
                        SCH_LINE* line = static_cast<SCH_LINE*>( aSelectedItem );
                        m_specialCaseSheetPins[ pin ] = { line, line->GetStartPoint() == aPoint };
                    }
                    else if( !newWire )
                    {
                        // Add a new wire between the sheetpin and the selected item so the
                        // selected item can be dragged.
                        newWire = makeNewWire( aCommit, pin, aSelectedItem, aPoint, aPoint );
                        newWire->SetFlags( SELECTED_BY_DRAG | STARTPOINT );
                        aList.push_back( newWire );
                    }
                }
            }

            break;

        case SCH_SYMBOL_T:
        case SCH_JUNCTION_T:
            if( test->IsConnected( aPoint ) && !newWire )
            {
                // Add a new wire between the symbol or junction and the selected item so
                // the selected item can be dragged.
                newWire = makeNewWire( aCommit, test, aSelectedItem, aPoint, aPoint );
                newWire->SetFlags( SELECTED_BY_DRAG | STARTPOINT );
                aList.push_back( newWire );
            }

            break;

        case SCH_NO_CONNECT_T:
            // Select no-connects that are connected to items being moved.
            if( !test->HasFlag( SELECTED_BY_DRAG ) && test->IsConnected( aPoint ) )
            {
                aList.push_back( test );
                test->SetFlags( SELECTED_BY_DRAG );
            }

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        case SCH_SHEET_PIN_T:
            // Performance optimization:
            if( test->HasFlag( SELECTED_BY_DRAG ) )
                break;

            // Select labels that are connected to a wire (or bus) being moved.
            if( aSelectedItem->Type() == SCH_LINE_T && test->CanConnect( aSelectedItem ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( test );
                SCH_LINE*       line = static_cast<SCH_LINE*>( aSelectedItem );

                bool oneEndFixed = !line->HasFlag( STARTPOINT ) || !line->HasFlag( ENDPOINT );

                if( line->HitTest( label->GetTextPos(), 1 ) )
                {
                    if(    ( !line->HasFlag( STARTPOINT ) && label->GetPosition() == line->GetStartPoint() )
                        || ( !line->HasFlag( ENDPOINT ) && label->GetPosition() == line->GetEndPoint() ) )
                    {
                        //If we have a line selected at only one end, don't grab labels
                        //connected directly to the unselected endpoint
                        break;
                    }
                    else
                    {
                        label->SetFlags( SELECTED_BY_DRAG );
                        aList.push_back( label );

                        if( oneEndFixed )
                        {
                            SPECIAL_CASE_LABEL_INFO info;
                            info.attachedLine = line;
                            info.originalLabelPos = label->GetPosition();
                            m_specialCaseLabels[label] = info;
                        }
                    }
                }
            }
            else if( test->IsConnected( aPoint ) && !newWire )
            {
                // Add a new wire between the label and the selected item so the selected item
                // can be dragged.
                newWire = makeNewWire( aCommit, test, aSelectedItem, aPoint, aPoint );
                newWire->SetFlags( SELECTED_BY_DRAG | STARTPOINT );
                aList.push_back( newWire );
            }

            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Performance optimization:
            if( test->HasFlag( SELECTED_BY_DRAG ) )
                break;

            // Select bus entries that are connected to a bus being moved.
            if( aSelectedItem->Type() == SCH_LINE_T && test->CanConnect( aSelectedItem ) )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( aSelectedItem );

                if(    ( !line->HasFlag( STARTPOINT ) && test->IsConnected( line->GetStartPoint() ) )
                    || ( !line->HasFlag( ENDPOINT ) && test->IsConnected( line->GetEndPoint() ) ) )
                {
                    // If we have a line selected at only one end, don't grab bus entries
                    // connected directly to the unselected endpoint
                    continue;
                }

                for( VECTOR2I& point : test->GetConnectionPoints() )
                {
                    if( line->HitTest( point, 1 ) )
                    {
                        test->SetFlags( SELECTED_BY_DRAG );
                        aList.push_back( test );

                        // A bus entry needs its wire & label as well
                        std::vector<VECTOR2I> ends = test->GetConnectionPoints();
                        VECTOR2I              otherEnd;

                        if( ends[0] == point )
                            otherEnd = ends[1];
                        else
                            otherEnd = ends[0];

                        getConnectedDragItems( aCommit, test, otherEnd, aList );

                        // No need to test the other end of the bus entry
                        break;
                    }
                }
            }

            break;

        default:
            break;
        }
    }
}


void SCH_MOVE_TOOL::moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

        if( aItem->HasFlag( STARTPOINT ) || !m_isDrag )
            line->MoveStart( aDelta );

        if( aItem->HasFlag( ENDPOINT ) || !m_isDrag )
            line->MoveEnd( aDelta );

        break;
    }

    case SCH_PIN_T:
    case SCH_FIELD_T:
    {
        SCH_ITEM* parent = (SCH_ITEM*) aItem->GetParent();
        VECTOR2I  delta( aDelta );

        if( parent && parent->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem->GetParent();
            TRANSFORM   transform = symbol->GetTransform().InverseTransform();

            delta = transform.TransformCoordinate( delta );
        }

        static_cast<SCH_ITEM*>( aItem )->Move( delta );

        // If we're moving a field with respect to its parent then it's no longer auto-placed
        if( aItem->Type() == SCH_FIELD_T && parent && !parent->IsSelected() )
            parent->SetFieldsAutoplaced( AUTOPLACE_NONE );

        break;
    }

    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN* pin = (SCH_SHEET_PIN*) aItem;

        pin->SetStoredPos( pin->GetStoredPos() + aDelta );
        pin->ConstrainOnEdge( pin->GetStoredPos(), true );
        break;
    }

    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    {
        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );

        if( m_specialCaseLabels.count( label ) )
        {
            SPECIAL_CASE_LABEL_INFO info = m_specialCaseLabels[ label ];
            SEG currentLine( info.attachedLine->GetStartPoint(), info.attachedLine->GetEndPoint() );
            label->SetPosition( currentLine.NearestPoint( info.originalLabelPos ) );
        }
        else
        {
            label->Move( aDelta );
        }

        break;
    }

    default:
        static_cast<SCH_ITEM*>( aItem )->Move( aDelta );
        break;
    }

    aItem->SetFlags( IS_MOVING );
}


int SCH_MOVE_TOOL::AlignToGrid( const TOOL_EVENT& aEvent )
{
    EE_GRID_HELPER    grid( m_toolMgr);
    SCH_SELECTION&    selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems );
    GRID_HELPER_GRIDS selectionGrid = grid.GetSelectionGrid( selection );
    SCH_COMMIT        commit( m_toolMgr );

    auto doMoveItem =
            [&]( EDA_ITEM* item, const VECTOR2I& delta )
            {
                commit.Modify( item, m_frame->GetScreen(), RECURSE_MODE::RECURSE );

                // Ensure only one end is moved when calling moveItem
                // i.e. we are in drag mode
                bool tmp_isDrag = m_isDrag;
                m_isDrag = true;
                moveItem( item, delta );
                m_isDrag = tmp_isDrag;

                item->ClearFlags( IS_MOVING );
                updateItem( item, true );
            };

    for( SCH_ITEM* it : m_frame->GetScreen()->Items() )
    {
        if( !it->IsSelected() )
            it->ClearFlags( STARTPOINT | ENDPOINT );

        if( !selection.IsHover() && it->IsSelected() )
            it->SetFlags( STARTPOINT | ENDPOINT );

        it->SetStoredPos( it->GetPosition() );

        if( it->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( it )->GetPins() )
                pin->SetStoredPos( pin->GetPosition() );
        }
    }

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE*             line = static_cast<SCH_LINE*>( item );
            std::vector<int>      flags{ STARTPOINT, ENDPOINT };
            std::vector<VECTOR2I> pts{ line->GetStartPoint(), line->GetEndPoint() };

            for( int ii = 0; ii < 2; ++ii )
            {
                EDA_ITEMS drag_items{ item };
                line->ClearFlags();
                line->SetFlags( SELECTED );
                line->SetFlags( flags[ii] );
                getConnectedDragItems( &commit, line, pts[ii], drag_items );
                std::set<EDA_ITEM*> unique_items( drag_items.begin(), drag_items.end() );

                VECTOR2I delta = grid.AlignGrid( pts[ii], selectionGrid ) - pts[ii];

                if( delta != VECTOR2I( 0, 0 ) )
                {
                    for( EDA_ITEM* dragItem : unique_items )
                    {
                        if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                            continue;

                        doMoveItem( dragItem, delta );
                    }
                }
            }
        }
        else if( item->Type() == SCH_FIELD_T || item->Type() == SCH_TEXT_T )
        {
            VECTOR2I delta = grid.AlignGrid( item->GetPosition(), selectionGrid ) - item->GetPosition();

            if( delta != VECTOR2I( 0, 0 ) )
                doMoveItem( item, delta );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
            VECTOR2I   topLeft = sheet->GetPosition();
            VECTOR2I   bottomRight = topLeft + sheet->GetSize();
            VECTOR2I   tl_delta = grid.AlignGrid( topLeft, selectionGrid ) - topLeft;
            VECTOR2I   br_delta = grid.AlignGrid( bottomRight, selectionGrid ) - bottomRight;

            if( tl_delta != VECTOR2I( 0, 0 ) || br_delta != VECTOR2I( 0, 0 ) )
            {
                doMoveItem( sheet, tl_delta );

                VECTOR2I newSize = (VECTOR2I) sheet->GetSize() - tl_delta + br_delta;
                sheet->SetSize( VECTOR2I( newSize.x, newSize.y ) );
                updateItem( sheet, true );
            }

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                VECTOR2I newPos;

                if( pin->GetSide() == SHEET_SIDE::TOP || pin->GetSide() == SHEET_SIDE::LEFT )
                    newPos = pin->GetPosition() + tl_delta;
                else
                    newPos = pin->GetPosition() + br_delta;

                VECTOR2I delta = grid.AlignGrid( newPos - pin->GetPosition(), selectionGrid );

                if( delta != VECTOR2I( 0, 0 ) )
                {
                    EDA_ITEMS drag_items;
                    getConnectedDragItems( &commit, pin, pin->GetConnectionPoints()[0],
                                           drag_items );

                    doMoveItem( pin, delta );

                    for( EDA_ITEM* dragItem : drag_items )
                    {
                        if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                            continue;

                        doMoveItem( dragItem, delta );
                    }
                }
            }
        }
        else
        {
            SCH_ITEM*             schItem = static_cast<SCH_ITEM*>( item );
            std::vector<VECTOR2I> connections = schItem->GetConnectionPoints();
            EDA_ITEMS             drag_items;

            for( const VECTOR2I& point : connections )
                getConnectedDragItems( &commit, schItem, point, drag_items );

            std::map<VECTOR2I, int> shifts;
            VECTOR2I                most_common( 0, 0 );
            int                     max_count = 0;

            for( const VECTOR2I& conn : connections )
            {
                VECTOR2I gridpt = grid.AlignGrid( conn, selectionGrid ) - conn;

                shifts[gridpt]++;

                if( shifts[gridpt] > max_count )
                {
                    most_common = gridpt;
                    max_count = shifts[most_common];
                }
            }

            if( most_common != VECTOR2I( 0, 0 ) )
            {
                doMoveItem( item, most_common );

                for( EDA_ITEM* dragItem : drag_items )
                {
                    if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                        continue;

                    doMoveItem( dragItem, most_common );
                }
            }
        }
    }

    SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
    lwbTool->TrimOverLappingWires( &commit, &selection );
    lwbTool->AddJunctionsIfNeeded( &commit, &selection );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );

    m_frame->Schematic().CleanUp( &commit );
    commit.Push( _( "Align Items to Grid" ) );
    return 0;
}


void SCH_MOVE_TOOL::clearNewDragLines()
{
    // Remove new bend lines added during the drag
    for( SCH_LINE* newLine : m_newDragLines )
    {
        m_frame->RemoveFromScreen( newLine, m_frame->GetScreen() );
        delete newLine;
    }

    m_newDragLines.clear();
}


void SCH_MOVE_TOOL::setTransitions()
{
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::move.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::drag.MakeEvent() );
    Go( &SCH_MOVE_TOOL::AlignToGrid,        SCH_ACTIONS::alignToGrid.MakeEvent() );
}


