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
#include <memory>
#include <optional>
#include <wx/log.h>
#include <trigo.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_drag_net_collision.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_move_tool.h>

#include <sch_actions.h>
#include <sch_commit.h>
#include <eda_item.h>
#include <sch_group.h>
#include <sch_item.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_line.h>
#include <sch_connection.h>
#include <sch_junction.h>
#include <junction_helpers.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <view/view_controls.h>
#include <settings/settings_manager.h>
#include <math/box2.h>
#include <base_units.h>
#include <sch_screen.h>
#include <trace_helpers.h>


// For adding to or removing from selections
#define QUIET_MODE true


static bool isGraphicItemForDrop( const SCH_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case SCH_SHAPE_T:
    case SCH_BITMAP_T:
    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
        return true;
    case SCH_LINE_T:
        return static_cast<const SCH_LINE*>( aItem )->IsGraphicLine();
    default:
        return false;
    }
}


static void cloneWireConnection( SCH_LINE* aNewLine, SCH_ITEM* aSource, SCH_EDIT_FRAME* aFrame )
{
    if( !aNewLine || !aSource || !aFrame )
        return;

    SCH_LINE* sourceLine = dynamic_cast<SCH_LINE*>( aSource );

    if( !sourceLine )
        return;

    SCH_SHEET_PATH sheetPath = aFrame->GetCurrentSheet();
    SCH_CONNECTION* sourceConnection = sourceLine->Connection( &sheetPath );

    if( !sourceConnection )
        return;

    SCH_CONNECTION* newConnection = aNewLine->InitializeConnection( sheetPath, nullptr );

    if( !newConnection )
        return;

    newConnection->Clone( *sourceConnection );
}


SCH_MOVE_TOOL::SCH_MOVE_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveMove" ),
        m_inMoveTool( false ),
        m_moveInProgress( false ),
        m_mode( MOVE ),
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


void SCH_MOVE_TOOL::Reset( RESET_REASON aReason )
{
    SCH_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD || aReason == SUPERMODEL_RELOAD )
    {
        // If we were in the middle of a move/drag operation and the model changes (e.g., sheet
        // switch), we need to clean up our state to avoid blocking future move/drag operations
        if( m_moveInProgress )
        {
            // Clear the move state
            m_moveInProgress = false;
            m_mode = MOVE;
            m_moveOffset = VECTOR2I( 0, 0 );
            m_anchorPos.reset();
            m_breakPos.reset();

            // Clear cached data that references items from the previous sheet
            m_dragAdditions.clear();
            m_lineConnectionCache.clear();
            m_newDragLines.clear();
            m_changedDragLines.clear();
            m_specialCaseLabels.clear();
            m_specialCaseSheetPins.clear();
            m_hiddenJunctions.clear();

            // Clear any preview
            if( m_view )
                m_view->ClearPreview();
        }
    }
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
            cloneWireConnection( foundLine, line, m_frame );
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

                // Re-attach drag labels from lines being deleted to the surviving line.
                // This prevents dangling pointers when bendLine/foundLine are deleted below.
                for( auto& [label, info] : m_specialCaseLabels )
                {
                    if( info.attachedLine == bendLine || info.attachedLine == foundLine )
                        info.attachedLine = line;
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
            cloneWireConnection( a, line, m_frame );
            m_frame->AddToScreen( a, m_frame->GetScreen() );
            m_newDragLines.insert( a );

            SCH_LINE* b = new SCH_LINE( a->GetStartPoint(), line->GetLayer() );
            b->MoveStart( VECTOR2I( splitDelta.x, splitDelta.y ) );
            b->SetFlags( IS_NEW | STARTPOINT );
            b->SetConnectivityDirty( true );
            b->SetLastResolvedState( line );
            cloneWireConnection( b, line, m_frame );
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
    if( aEvent.IsAction( &SCH_ACTIONS::drag ) )
        m_mode = DRAG;
    else if( aEvent.IsAction( &SCH_ACTIONS::breakWire ) )
        m_mode = BREAK;
    else if( aEvent.IsAction( &SCH_ACTIONS::slice ) )
        m_mode = SLICE;
    else
        m_mode = MOVE;

    if( SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() ) )
    {
        wxCHECK( aEvent.SynchronousState(), 0 );
        aEvent.SynchronousState()->store( STS_RUNNING );

        if( doMoveSelection( aEvent, commit ) )
            aEvent.SynchronousState()->store( STS_FINISHED );
        else
            aEvent.SynchronousState()->store( STS_CANCELLED );
    }
    else
    {
        SCH_COMMIT localCommit( m_toolMgr );

        if( doMoveSelection( aEvent, &localCommit ) )
        {
            switch( m_mode )
            {
            case MOVE:  localCommit.Push( _( "Move" ) );       break;
            case DRAG:  localCommit.Push( _( "Drag" ) );       break;
            case BREAK: localCommit.Push( _( "Break Wire" ) ); break;
            case SLICE: localCommit.Push( _( "Slice Wire" ) ); break;
            }
        }
        else
        {
            localCommit.Revert();
        }
    }

    return 0;
}


void SCH_MOVE_TOOL::preprocessBreakOrSliceSelection( SCH_COMMIT* aCommit, const TOOL_EVENT& aEvent )
{
    if( m_mode != BREAK && m_mode != SLICE )
        return;

    if( !aCommit )
        return;

    SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();

    if( !lwbTool )
        return;

    SCH_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return;

    std::vector<SCH_LINE*> lines;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_LINE_T )
        {
            // This function gets called every time segments are broken, which can also be for subsequent
            // breaks in a loop without leaving the current move tool.
            // Skip already placed segments (segment keeps IS_BROKEN but will have IS_NEW cleared below)
            // so that only the actively placed tail segment gets split again.
            if( item->HasFlag( IS_BROKEN ) && !item->HasFlag( IS_NEW ) )
                continue;

            lines.push_back( static_cast<SCH_LINE*>( item ) );
        }
    }

    if( lines.empty() )
        return;

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    SCH_SCREEN*           screen = m_frame->GetScreen();
    VECTOR2I              cursorPos = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );

    bool useCursorForSingleLine = false;

    if( lines.size() == 1 )
        useCursorForSingleLine = true;

    m_selectionTool->ClearSelection();
    m_breakPos.reset();

    for( SCH_LINE* line : lines )
    {
        VECTOR2I  breakPos = useCursorForSingleLine ? cursorPos : line->GetMidPoint();

        if( m_mode == BREAK && !m_breakPos )
            m_breakPos = breakPos;

        SCH_LINE* newLine = nullptr;

        lwbTool->BreakSegment( aCommit, line, breakPos, &newLine, screen );

        if( !newLine )
            continue;

        // If this is a second+ round break, we need to get rid of the IS_NEW flag since the new segment
        // is now an existing segment we are breaking from, this will be checked for in the line selection
        // gathering above
        line->ClearFlags( STARTPOINT | IS_NEW );
        line->SetFlags( ENDPOINT );
        m_selectionTool->AddItemToSel( line );

        newLine->ClearFlags( ENDPOINT | STARTPOINT );

        if( m_mode == BREAK )
        {
            m_selectionTool->AddItemToSel( newLine );
            newLine->SetFlags( STARTPOINT );
        }
    }
}


bool SCH_MOVE_TOOL::doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    bool                  currentModeIsDragLike = ( m_mode != MOVE );
    bool                  wasDragging = m_moveInProgress && currentModeIsDragLike;
    bool                  didAtLeastOneBreak = false;

    m_anchorPos.reset();

    // Check if already in progress and handle state transitions
    if( checkMoveInProgress( aEvent, aCommit, currentModeIsDragLike, wasDragging ) )
        return false;

    if( m_inMoveTool )      // Must come after m_moveInProgress checks above...
        return false;

    REENTRANCY_GUARD guard( &m_inMoveTool );

    preprocessBreakOrSliceSelection( aCommit, aEvent );

    // Prepare selection (promote pins to symbols, request selection)
    bool           unselect = false;
    SCH_SELECTION& selection = prepareSelection( unselect );

    // Keep an original copy of the starting points for cleanup after the move
    std::vector<DANGLING_END_ITEM> internalPoints;

    // Track selection characteristics
    bool selectionHasSheetPins = false;
    bool selectionHasGraphicItems = false;
    bool selectionHasNonGraphicItems = false;
    bool selectionIsGraphicsOnly = false;

    std::unique_ptr<SCH_DRAG_NET_COLLISION_MONITOR> netCollisionMonitor;

    auto refreshTraits =
            [&]()
            {
                refreshSelectionTraits( selection, selectionHasSheetPins, selectionHasGraphicItems,
                                        selectionHasNonGraphicItems, selectionIsGraphicsOnly );
            };

    refreshTraits();

    if( !selection.Empty() )

    {
        netCollisionMonitor = std::make_unique<SCH_DRAG_NET_COLLISION_MONITOR>( m_frame, m_view );
        netCollisionMonitor->Initialize( selection );
    }

    bool lastCtrlDown = false;

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
    VECTOR2I    prevPos = controls->GetCursorPosition();
    GRID_HELPER_GRIDS snapLayer = GRID_CURRENT;
    SCH_SHEET*  hoverSheet = nullptr;
    KICURSOR    currentCursor = KICURSOR::MOVING;
    m_cursor = controls->GetCursorPosition();

    // Axis locking for arrow key movement
    enum class AXIS_LOCK { NONE, HORIZONTAL, VERTICAL };
    AXIS_LOCK axisLock = AXIS_LOCK::NONE;
    long      lastArrowKeyAction = 0;

    // Main loop: keep receiving events
    do
    {
        wxLogTrace( traceSchMove, "doMoveSelection: event loop iteration, evt=%s, action=%s",
                    evt->Category() == TC_MOUSE ? "MOUSE" :
                    evt->Category() == TC_KEYBOARD ? "KEYBOARD" :
                    evt->Category() == TC_COMMAND ? "COMMAND" : "OTHER",
                    evt->Format().c_str() );

        m_frame->GetCanvas()->SetCurrentCursor( currentCursor );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        bool ctrlDown = evt->Modifier( MD_CTRL );
        lastCtrlDown = ctrlDown;

        if( evt->IsAction( &SCH_ACTIONS::restartMove )
                || evt->IsAction( &SCH_ACTIONS::move )
                || evt->IsAction( &SCH_ACTIONS::drag )
                || evt->IsMotion()
                || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            refreshTraits();

            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                initializeMoveOperation( aEvent, selection, aCommit, internalPoints, snapLayer );
                prevPos = m_cursor;
                refreshTraits();
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_view->ClearPreview();

            // We need to bypass refreshPreview action here because it is triggered by the move, so we were
            // getting double-key events that toggled the axis locking if you pressed them in a certain order.
            if( controls->GetSettings().m_lastKeyboardCursorPositionValid && !evt->IsAction( &ACTIONS::refreshPreview ) )
            {
                VECTOR2I keyboardPos( controls->GetSettings().m_lastKeyboardCursorPosition );
                long action = controls->GetSettings().m_lastKeyboardCursorCommand;

                grid.SetSnap( false );
                m_cursor = grid.Align( keyboardPos, snapLayer );

                // Update axis lock based on arrow key press
                if( action == ACTIONS::CURSOR_LEFT || action == ACTIONS::CURSOR_RIGHT )
                {
                    if( axisLock == AXIS_LOCK::HORIZONTAL )
                    {
                        // Check if opposite horizontal key pressed to unlock
                        if( ( lastArrowKeyAction == ACTIONS::CURSOR_LEFT && action == ACTIONS::CURSOR_RIGHT ) ||
                            ( lastArrowKeyAction == ACTIONS::CURSOR_RIGHT && action == ACTIONS::CURSOR_LEFT ) )
                        {
                            axisLock = AXIS_LOCK::NONE;
                        }
                        // Same direction axis, keep locked
                    }
                    else
                    {
                        axisLock = AXIS_LOCK::HORIZONTAL;
                    }
                }
                else if( action == ACTIONS::CURSOR_UP || action == ACTIONS::CURSOR_DOWN )
                {
                    if( axisLock == AXIS_LOCK::VERTICAL )
                    {
                        // Check if opposite vertical key pressed to unlock
                        if( ( lastArrowKeyAction == ACTIONS::CURSOR_UP && action == ACTIONS::CURSOR_DOWN ) ||
                            ( lastArrowKeyAction == ACTIONS::CURSOR_DOWN && action == ACTIONS::CURSOR_UP ) )
                        {
                            axisLock = AXIS_LOCK::NONE;
                        }
                        // Same direction axis, keep locked
                    }
                    else
                    {
                        axisLock = AXIS_LOCK::VERTICAL;
                    }
                }

                lastArrowKeyAction = action;
            }
            else
            {
                m_cursor = grid.BestSnapAnchor( controls->GetCursorPosition( false ), snapLayer, selection );
            }

            if( axisLock == AXIS_LOCK::HORIZONTAL )
                m_cursor.y = prevPos.y;
            else if( axisLock == AXIS_LOCK::VERTICAL )
                m_cursor.x = prevPos.x;

            // Find potential target sheet for dropping
            SCH_SHEET* sheet = findTargetSheet( selection, m_cursor, selectionHasSheetPins,
                                                selectionIsGraphicsOnly, ctrlDown );

            if( sheet != hoverSheet )
            {
                hoverSheet = sheet;

                if( hoverSheet )
                {
                    hoverSheet->SetFlags( BRIGHTENED );
                    m_frame->UpdateItem( hoverSheet, false );
                }
            }

            currentCursor = hoverSheet ? KICURSOR::PLACE : KICURSOR::MOVING;

            if( netCollisionMonitor )
                currentCursor = netCollisionMonitor->AdjustCursor( currentCursor );

            VECTOR2I delta( m_cursor - prevPos );
            m_anchorPos = m_cursor;

            // Used for tracking how far off a drag end should have its 90 degree elbow added
            int xBendCount = 1;
            int yBendCount = 1;

            performItemMove( selection, delta, aCommit, xBendCount, yBendCount, grid );
            prevPos = m_cursor;

            std::vector<SCH_ITEM*> previewItems;

            for( EDA_ITEM* it : selection )
                previewItems.push_back( static_cast<SCH_ITEM*>( it ) );

            for( SCH_LINE* line : m_newDragLines )
                previewItems.push_back( line );

            for( SCH_LINE* line : m_changedDragLines )
                previewItems.push_back( line );

            std::vector<SCH_JUNCTION*> previewJunctions =
                    JUNCTION_HELPERS::PreviewJunctions( m_frame->GetScreen(), previewItems );

            if( netCollisionMonitor )
                netCollisionMonitor->Update( previewJunctions, selection );

            for( SCH_JUNCTION* jct : previewJunctions )
                m_view->AddToPreview( jct, true );

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
            {
                m_frame->GetInfoBar()->Dismiss();

                // When breaking, the user can cancel after multiple breaks to keep all but the last
                // break, so exit normally if we have done at least one break
                if( didAtLeastOneBreak && m_mode == BREAK )
                    break;
            }

            if( m_moveInProgress )
            {
                if( evt->IsActivate() )
                {
                    // Allowing other tools to activate during a move runs the risk of race
                    // conditions in which we try to spool up both event loops at once.

                    switch( m_mode )
                    {
                    case MOVE:  m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel move." ) );  break;
                    case DRAG:  m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel drag." ) );  break;
                    case BREAK: m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel break." ) ); break;
                    case SLICE: m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel slice." ) ); break;
                    }

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
        else if( !handleMoveToolActions( evt, aCommit, selection ) )
        {
            wxLogTrace( traceSchMove, "doMoveSelection: handleMoveToolActions returned false, exiting" );
            break;  // Exit if told to by handler
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
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            if( m_mode != BREAK )
                break; // Finish
            else
            {
                didAtLeastOneBreak = true;
                preprocessBreakOrSliceSelection( aCommit, *evt );
                selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems, true );

                if( m_breakPos )
                {
                    m_cursor = *m_breakPos;
                    m_anchorPos = m_cursor;
                    selection.SetReferencePoint( m_cursor );
                    m_moveOffset = VECTOR2I( 0, 0 );
                    m_breakPos.reset();

                    controls->SetCursorPosition( m_cursor, false );
                    prevPos = m_cursor;
                }
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            // Double click always finishes, even breaks
            break;
        }
        // Don't call SetPassEvent() for events we've handled - let them be consumed
        else if( evt->IsAction( &SCH_ACTIONS::rotateCW )
                 || evt->IsAction( &SCH_ACTIONS::rotateCCW )
                 || evt->IsAction( &ACTIONS::increment )
                 || evt->IsAction( &SCH_ACTIONS::toDLabel )
                 || evt->IsAction( &SCH_ACTIONS::toGLabel )
                 || evt->IsAction( &SCH_ACTIONS::toHLabel )
                 || evt->IsAction( &SCH_ACTIONS::toLabel )
                 || evt->IsAction( &SCH_ACTIONS::toText )
                 || evt->IsAction( &SCH_ACTIONS::toTextBox )
                 || evt->IsAction( &SCH_ACTIONS::highlightNet )
                 || evt->IsAction( &SCH_ACTIONS::selectOnPCB )
                 || evt->IsAction( &ACTIONS::duplicate )
                 || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                 || evt->IsAction( &ACTIONS::redo ) )
        {
            // Event was already handled by handleMoveToolActions, don't pass it on
            wxLogTrace( traceSchMove, "doMoveSelection: event handled, not passing" );
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( m_moveInProgress );

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    SCH_SHEET* targetSheet = hoverSheet;

    if( selectionHasSheetPins || ( selectionIsGraphicsOnly && !lastCtrlDown ) )
        targetSheet = nullptr;

    if( hoverSheet )
    {
        hoverSheet->ClearFlags( BRIGHTENED );
        m_frame->UpdateItem( hoverSheet, false );
    }

    if( restore_state )
    {
        m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
    }
    else
    {
        // Only drop into a sheet when the move is committed, not when canceled.
        if( targetSheet )
        {
            moveSelectionToSheet( selection, targetSheet, aCommit );
            m_toolMgr->RunAction( ACTIONS::selectionClear );
            m_newDragLines.clear();
            m_changedDragLines.clear();
        }

        finalizeMoveOperation( selection, aCommit, unselect, internalPoints );
    }

    m_dragAdditions.clear();
    m_lineConnectionCache.clear();
    m_moveInProgress = false;
    m_breakPos.reset();

    m_hiddenJunctions.clear();
    m_view->ClearPreview();
    m_frame->PopTool( aEvent );

    return !restore_state;
}


bool SCH_MOVE_TOOL::checkMoveInProgress( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit, bool aCurrentModeIsDragLike,
                                         bool aWasDragging )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( !m_moveInProgress )
        return false;

    if( aCurrentModeIsDragLike != aWasDragging )
    {
        EDA_ITEM* sel = m_selectionTool->GetSelection().Front();

        if( sel && !sel->IsNew() )
        {
            // Reset the selected items so we can start again with the current drag mode state
            aCommit->Revert();

            m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
            m_anchorPos = m_cursor - m_moveOffset;
            m_moveInProgress = false;
            controls->SetAutoPan( false );

            // Give it a kick so it doesn't have to wait for the first mouse movement to refresh
            m_toolMgr->PostAction( SCH_ACTIONS::restartMove );
        }
    }
    else
    {
        // The tool hotkey is interpreted as a click when already dragging/moving
        m_toolMgr->PostAction( ACTIONS::cursorClick );
    }

    return true;
}


SCH_SELECTION& SCH_MOVE_TOOL::prepareSelection( bool& aUnselect )
{
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
    // looking for the stuff under mouse cursor (i.e. KiCad old-style hover selection).
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems, true );
    aUnselect = selection.IsHover();

    return selection;
}


void SCH_MOVE_TOOL::refreshSelectionTraits( const SCH_SELECTION& aSelection, bool& aHasSheetPins,
                                            bool& aHasGraphicItems, bool& aHasNonGraphicItems,
                                            bool& aIsGraphicsOnly )
{
    aHasSheetPins = false;
    aHasGraphicItems = false;
    aHasNonGraphicItems = false;

    for( EDA_ITEM* edaItem : aSelection )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( edaItem );

        if( schItem->Type() == SCH_SHEET_PIN_T )
            aHasSheetPins = true;

        if( isGraphicItemForDrop( schItem ) )
            aHasGraphicItems = true;
        else if( schItem->Type() != SCH_SHEET_T )
            aHasNonGraphicItems = true;
    }

    aIsGraphicsOnly = aHasGraphicItems && !aHasNonGraphicItems;
}


void SCH_MOVE_TOOL::setupItemsForDrag( SCH_SELECTION& aSelection, SCH_COMMIT* aCommit )
{
    // Drag of split items start over top of their other segment, so we want to skip grabbing
    // the segments we split from
    if( m_mode != DRAG && m_mode != BREAK )
        return;

    EDA_ITEMS connectedDragItems;

    // Add connections to the selection for a drag.
    // Do all non-labels/entries first so we don't add junctions to drag when the line will
    // eventually be drag selected.
    std::vector<SCH_ITEM*> stageTwo;

    for( EDA_ITEM* edaItem : aSelection )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
        std::vector<VECTOR2I> connections;

        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            stageTwo.emplace_back( item );
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

    // Go back and get all label connections now that we can test for drag-selected lines
    // the labels might be on
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

    // Pre-cache all connections of our selected objects so we can keep track of what they
    // were originally connected to as we drag them around
    for( EDA_ITEM* edaItem : aSelection )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( edaItem );

        if( schItem->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( schItem );

            // Store the original angle of the line; needed later to decide which segment
            // to extend when they've become zero length
            line->StoreAngle();

            for( const VECTOR2I& point : line->GetConnectionPoints() )
                getConnectedItems( line, point, m_lineConnectionCache[line] );
        }
    }
}


void SCH_MOVE_TOOL::setupItemsForMove( SCH_SELECTION& aSelection, std::vector<DANGLING_END_ITEM>& aInternalPoints )
{
    // Mark the edges of the block with dangling flags for a move
    for( EDA_ITEM* item : aSelection )
        static_cast<SCH_ITEM*>( item )->GetEndPoints( aInternalPoints );

    std::vector<DANGLING_END_ITEM> endPointsByType = aInternalPoints;
    std::vector<DANGLING_END_ITEM> endPointsByPos = endPointsByType;
    DANGLING_END_ITEM_HELPER::sort_dangling_end_items( endPointsByType, endPointsByPos );

    for( EDA_ITEM* item : aSelection )
        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( endPointsByType, endPointsByPos );
}


void SCH_MOVE_TOOL::initializeMoveOperation( const TOOL_EVENT& aEvent, SCH_SELECTION& aSelection, SCH_COMMIT* aCommit,
                                             std::vector<DANGLING_END_ITEM>& aInternalPoints,
                                             GRID_HELPER_GRIDS&              aSnapLayer )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    SCH_ITEM*             sch_item = static_cast<SCH_ITEM*>( aSelection.Front() );
    bool                  placingNewItems = sch_item && sch_item->IsNew();

    //------------------------------------------------------------------------
    // Setup a drag or a move
    //
    m_dragAdditions.clear();
    m_specialCaseLabels.clear();
    m_specialCaseSheetPins.clear();
    aInternalPoints.clear();
    clearNewDragLines();

    for( SCH_ITEM* it : m_frame->GetScreen()->Items() )
    {
        it->ClearFlags( SELECTED_BY_DRAG );

        if( !it->IsSelected() )
            it->ClearFlags( STARTPOINT | ENDPOINT );
    }

    setupItemsForDrag( aSelection, aCommit );
    setupItemsForMove( aSelection, aInternalPoints );

    // Hide junctions connected to line endpoints that are not selected
    m_hiddenJunctions.clear();

    for( EDA_ITEM* item : aSelection )
        item->SetFlags( STRUCT_DELETED );

    for( EDA_ITEM* edaItem : aSelection )
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
                JUNCTION_HELPERS::POINT_INFO info = JUNCTION_HELPERS::AnalyzePoint( m_frame->GetScreen()->Items(),
                                                                                    pt, false );

                if( !info.isJunction )
                {
                    jct->SetFlags( STRUCT_DELETED );
                    m_frame->RemoveFromScreen( jct, m_frame->GetScreen() );
                    aCommit->Removed( jct, m_frame->GetScreen() );
                }
            }
        }
    }

    for( EDA_ITEM* item : aSelection )
        item->ClearFlags( STRUCT_DELETED );

    // Generic setup
    aSnapLayer = grid.GetSelectionGrid( aSelection );

    for( EDA_ITEM* item : aSelection )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        if( schItem->IsNew() )
        {
            // Item was added to commit in a previous command

            // While SCH_COMMIT::Push() will add any new items to the entered group, we need
            // to do it earlier so that the previews while moving are correct.
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
                [&]( SCH_ITEM* unused )
                {
                    item->SetFlags( IS_MOVING );
                },
                RECURSE_MODE::RECURSE );

        schItem->SetStoredPos( schItem->GetPosition() );
    }

    // Set up the starting position and move/drag offset
    m_cursor = controls->GetCursorPosition();

    if( m_mode == BREAK && m_breakPos )
    {
        m_cursor = *m_breakPos;
        m_anchorPos = m_cursor;
        aSelection.SetReferencePoint( m_cursor );
        m_moveOffset = VECTOR2I( 0, 0 );
        m_breakPos.reset();
    }

    if( aEvent.IsAction( &SCH_ACTIONS::restartMove ) )
    {
        wxASSERT_MSG( m_anchorPos, "Should be already set from previous cmd" );
    }
    else if( placingNewItems )
    {
        m_anchorPos = aSelection.GetReferencePoint();
    }

    if( m_anchorPos )
    {
        VECTOR2I delta = m_cursor - ( *m_anchorPos );
        bool     isPasted = false;

        // Drag items to the current cursor position
        for( EDA_ITEM* item : aSelection )
        {
            // Don't double move pins, fields, etc.
            if( item->GetParent() && item->GetParent()->IsSelected() )
                continue;

            moveItem( item, delta );
            updateItem( item, false );

            isPasted |= ( item->GetFlags() & IS_PASTED ) != 0;
        }

        // The first time pasted items are moved we need to store the position of the cursor
        // so that rotate while moving works as expected (instead of around the original
        // anchor point)
        if( isPasted )
            aSelection.SetReferencePoint( m_cursor );

        m_anchorPos = m_cursor;
    }
    // For some items, moving the cursor to anchor is not good (for instance large
    // hierarchical sheets or symbols can have the anchor outside the view)
    else if( aSelection.Size() == 1 && !sch_item->IsMovableFromAnchorPoint() )
    {
        m_cursor = getViewControls()->GetCursorPosition( true );
        m_anchorPos = m_cursor;
    }
    else
    {
        if( m_frame->GetMoveWarpsCursor() )
        {
            // User wants to warp the mouse
            m_cursor = grid.BestDragOrigin( m_cursor, aSnapLayer, aSelection );
            aSelection.SetReferencePoint( m_cursor );
        }
        else
        {
            // User does not want to warp the mouse
            m_cursor = getViewControls()->GetCursorPosition( true );
        }
    }

    controls->SetCursorPosition( m_cursor, false );
    controls->SetAutoPan( true );
    m_moveInProgress = true;
}


SCH_SHEET* SCH_MOVE_TOOL::findTargetSheet( const SCH_SELECTION& aSelection, const VECTOR2I& aCursorPos,
                                           bool aHasSheetPins, bool aIsGraphicsOnly, bool aCtrlDown )
{
    // Determine potential target sheet
    SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( m_frame->GetScreen()->GetItem( aCursorPos, 0, SCH_SHEET_T ) );

    if( sheet && sheet->IsSelected() )
        sheet = nullptr;  // Never target a selected sheet

    if( !sheet )
    {
        // Build current selection bounding box in its (already moved) position
        BOX2I selBBox;

        for( EDA_ITEM* it : aSelection )
        {
            if( SCH_ITEM* schIt = dynamic_cast<SCH_ITEM*>( it ) )
                selBBox.Merge( schIt->GetBoundingBox() );
        }

        if( selBBox.GetWidth() > 0 && selBBox.GetHeight() > 0 )
        {
            VECTOR2I selCenter( selBBox.GetX() + selBBox.GetWidth() / 2,
                                selBBox.GetY() + selBBox.GetHeight() / 2 );

            // Find first non-selected sheet whose body fully contains the selection or at
            // least contains its center point
            for( SCH_ITEM* it : m_frame->GetScreen()->Items().OfType( SCH_SHEET_T ) )
            {
                SCH_SHEET* candidate = static_cast<SCH_SHEET*>( it );

                if( candidate->IsSelected() || candidate->IsTopLevelSheet() )
                    continue;

                BOX2I body = candidate->GetBodyBoundingBox();

                if( body.Contains( selBBox ) || body.Contains( selCenter ) )
                {
                    sheet = candidate;
                    break;
                }
            }
        }
    }

    bool dropAllowedBySelection = !aHasSheetPins;
    bool dropAllowedByModifiers = !aIsGraphicsOnly || aCtrlDown;

    if( sheet && !( dropAllowedBySelection && dropAllowedByModifiers ) )
        sheet = nullptr;

    return sheet;
}


void SCH_MOVE_TOOL::performItemMove( SCH_SELECTION& aSelection, const VECTOR2I& aDelta,
                                     SCH_COMMIT* aCommit, int& aXBendCount, int& aYBendCount,
                                     const EE_GRID_HELPER& aGrid )
{
    wxLogTrace( traceSchMove, "performItemMove: delta=(%d,%d), moveOffset=(%d,%d), selection size=%u",
                aDelta.x, aDelta.y, m_moveOffset.x, m_moveOffset.y, aSelection.GetSize() );

    // We need to check if the movement will change the net offset direction on the X and Y
    // axes. This is because we remerge added bend lines in realtime, and we also account for
    // the direction of the move when adding bend lines. So, if the move direction changes,
    // we need to split it into a move that gets us back to zero, then the rest of the move.
    std::vector<VECTOR2I> splitMoves;

    if( alg::signbit( m_moveOffset.x ) != alg::signbit( ( m_moveOffset + aDelta ).x ) )
    {
        splitMoves.emplace_back( VECTOR2I( -1 * m_moveOffset.x, 0 ) );
        splitMoves.emplace_back( VECTOR2I( aDelta.x + m_moveOffset.x, 0 ) );
    }
    else
    {
        splitMoves.emplace_back( VECTOR2I( aDelta.x, 0 ) );
    }

    if( alg::signbit( m_moveOffset.y ) != alg::signbit( ( m_moveOffset + aDelta ).y ) )
    {
        splitMoves.emplace_back( VECTOR2I( 0, -1 * m_moveOffset.y ) );
        splitMoves.emplace_back( VECTOR2I( 0, aDelta.y + m_moveOffset.y ) );
    }
    else
    {
        splitMoves.emplace_back( VECTOR2I( 0, aDelta.y ) );
    }

    m_moveOffset += aDelta;

    // Split the move into X and Y moves so we can correctly drag orthogonal lines
    for( const VECTOR2I& splitDelta : splitMoves )
    {
        // Skip non-moves
        if( splitDelta == VECTOR2I( 0, 0 ) )
            continue;

        for( EDA_ITEM* item : aSelection.GetItemsSortedByTypeAndXY( ( aDelta.x >= 0 ),
                                                                    ( aDelta.y >= 0 ) ) )
        {
            // Don't double move pins, fields, etc.
            if( item->GetParent() && item->GetParent()->IsSelected() )
                continue;

            SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );
            bool      isLineModeConstrained = false;

            if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
                isLineModeConstrained = cfg->m_Drawing.line_mode != LINE_MODE::LINE_MODE_FREE;

            // Only partially selected drag lines in orthogonal line mode need special handling
            if( ( m_mode == DRAG ) && isLineModeConstrained && line
                && line->HasFlag( STARTPOINT ) != line->HasFlag( ENDPOINT ) )
            {
                orthoLineDrag( aCommit, line, splitDelta, aXBendCount, aYBendCount, aGrid );
            }

            // Move all other items normally, including the selected end of partially selected
            // lines
            moveItem( item, splitDelta );
            updateItem( item, false );

            // Update any lines connected to sheet pins to the sheet pin's location (which may
            // not exactly follow the splitDelta as the pins are constrained along the sheet
            // edges)
            for( const auto& [pin, lineEnd] : m_specialCaseSheetPins )
            {
                if( lineEnd.second && lineEnd.first->HasFlag( STARTPOINT ) )
                    lineEnd.first->SetStartPoint( pin->GetPosition() );
                else if( !lineEnd.second && lineEnd.first->HasFlag( ENDPOINT ) )
                    lineEnd.first->SetEndPoint( pin->GetPosition() );
            }
        }
    }

    if( aSelection.HasReferencePoint() )
        aSelection.SetReferencePoint( aSelection.GetReferencePoint() + aDelta );
}


bool SCH_MOVE_TOOL::handleMoveToolActions( const TOOL_EVENT* aEvent, SCH_COMMIT* aCommit,
                                            const SCH_SELECTION& aSelection )
{
    wxLogTrace( traceSchMove, "handleMoveToolActions: received event, action=%s",
                aEvent->Format().c_str() );

    if( aEvent->IsAction( &ACTIONS::doDelete ) )
    {
        wxLogTrace( traceSchMove, "handleMoveToolActions: doDelete, exiting move" );
        const_cast<TOOL_EVENT*>( aEvent )->SetPassEvent();
        return false;  // Exit on delete; there will no longer be anything to drag
    }
    else if( aEvent->IsAction( &ACTIONS::duplicate )
             || aEvent->IsAction( &SCH_ACTIONS::repeatDrawItem )
             || aEvent->IsAction( &ACTIONS::redo ) )
    {
        wxBell();
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::rotateCW ) )
    {
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCW event received, selection size=%u",
                    aSelection.GetSize() );
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::rotateCW, aCommit );
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCW RunSynchronousAction completed" );
        updateStoredPositions( aSelection );
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCW updateStoredPositions completed" );
        // Note: SCH_EDIT_TOOL::Rotate already posts refreshPreview when moving
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::rotateCCW ) )
    {
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCCW event received, selection size=%u",
                    aSelection.GetSize() );
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::rotateCCW, aCommit );
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCCW RunSynchronousAction completed" );
        updateStoredPositions( aSelection );
        wxLogTrace( traceSchMove, "handleMoveToolActions: rotateCCW updateStoredPositions completed" );
        // Note: SCH_EDIT_TOOL::Rotate already posts refreshPreview when moving
    }
    else if( aEvent->IsAction( &ACTIONS::increment ) )
    {
        if( aEvent->HasParameter() )
            m_toolMgr->RunSynchronousAction( ACTIONS::increment, aCommit, aEvent->Parameter<ACTIONS::INCREMENT>() );
        else
            m_toolMgr->RunSynchronousAction( ACTIONS::increment, aCommit, ACTIONS::INCREMENT{ 1, 0 } );

        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toDLabel ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toDLabel, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toGLabel ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toGLabel, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toHLabel ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toHLabel, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toLabel ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toLabel, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toText ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toText, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::toTextBox ) )
    {
        m_toolMgr->RunSynchronousAction( SCH_ACTIONS::toTextBox, aCommit );
        updateStoredPositions( aSelection );
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent->Action() == TA_CHOICE_MENU_CHOICE )
    {
        if( *aEvent->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT
            && *aEvent->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_END )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selectionTool->GetSelection().Front() );
            int unit = *aEvent->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT;

            if( symbol )
            {
                m_frame->SelectUnit( symbol, unit );
                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
        }
        else if( *aEvent->GetCommandId() >= ID_POPUP_SCH_SELECT_BODY_STYLE
                 && *aEvent->GetCommandId() <= ID_POPUP_SCH_SELECT_BODY_STYLE_END )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selectionTool->GetSelection().Front() );
            int bodyStyle = ( *aEvent->GetCommandId() - ID_POPUP_SCH_SELECT_BODY_STYLE ) + 1;

            if( symbol && symbol->GetBodyStyle() != bodyStyle )
            {
                m_frame->SelectBodyStyle( symbol, bodyStyle );
                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
        }
    }
    else if( aEvent->IsAction( &SCH_ACTIONS::highlightNet )
             || aEvent->IsAction( &SCH_ACTIONS::selectOnPCB ) )
    {
        // These don't make any sense during a move. Eat them.
    }
    else
    {
        return true;  // Continue processing
    }

    return true;  // Continue processing
}


void SCH_MOVE_TOOL::updateStoredPositions( const SCH_SELECTION& aSelection )
{
    wxLogTrace( traceSchMove, "updateStoredPositions: start, selection size=%u",
                aSelection.GetSize() );

    // After transformations like rotation during a move, we need to update the stored
    // positions that moveItem() uses, particularly for sheet pins which rely on them
    // for constraint calculations.
    int itemCount = 0;

    for( EDA_ITEM* item : aSelection )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( item );

        if( !schItem )
            continue;

        VECTOR2I oldPos = schItem->GetStoredPos();
        VECTOR2I newPos = schItem->GetPosition();
        schItem->SetStoredPos( newPos );

        wxLogTrace( traceSchMove, "  item[%d] type=%d: stored pos updated (%d,%d) -> (%d,%d)",
                    itemCount++, (int) schItem->Type(), oldPos.x, oldPos.y, newPos.x, newPos.y );

        // Also update stored positions for sheet pins
        if( schItem->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( schItem );
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                VECTOR2I pinOldPos = pin->GetStoredPos();
                VECTOR2I pinNewPos = pin->GetPosition();
                pin->SetStoredPos( pinNewPos );
                wxLogTrace( traceSchMove, "    sheet pin: stored pos updated (%d,%d) -> (%d,%d)",
                            pinOldPos.x, pinOldPos.y, pinNewPos.x, pinNewPos.y );
            }
        }
    }

    wxLogTrace( traceSchMove, "updateStoredPositions: complete, updated %d items", itemCount );
}


void SCH_MOVE_TOOL::finalizeMoveOperation( SCH_SELECTION& aSelection, SCH_COMMIT* aCommit, bool aUnselect,
                                           const std::vector<DANGLING_END_ITEM>& aInternalPoints )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    const bool            isSlice = ( m_mode == SLICE );
    const bool            isDragLike = ( m_mode == DRAG || m_mode == BREAK );

    // Save whatever new bend lines and changed lines survived the drag
    for( SCH_LINE* newLine : m_newDragLines )
    {
        newLine->ClearEditFlags();
        aCommit->Added( newLine, m_frame->GetScreen() );
    }

    // These lines have been changed, but aren't selected. We need to manually clear these
    // edit flags or they'll stick around.
    for( SCH_LINE* oldLine : m_changedDragLines )
        oldLine->ClearEditFlags();

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_moveOffset = { 0, 0 };
    m_anchorPos.reset();

    // One last update after exiting loop (for slower stuff, such as updating SCREEN's RTree)
    for( EDA_ITEM* item : aSelection )
    {
        updateItem( item, true );

        if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item ) )
            sch_item->SetConnectivityDirty( true );
    }

    if( aSelection.GetSize() == 1 && aSelection.Front()->IsNew() )
        m_frame->SaveCopyForRepeatItem( static_cast<SCH_ITEM*>( aSelection.Front() ) );

    m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );

    SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();

    // If we move items away from a junction, we _may_ want to add a junction there
    // to denote the state
    for( const DANGLING_END_ITEM& it : aInternalPoints )
    {
        if( m_frame->GetScreen()->IsExplicitJunctionNeeded( it.GetPosition() ) )
            lwbTool->AddJunction( aCommit, m_frame->GetScreen(), it.GetPosition() );
    }

    // Create a selection of original selection, drag selected/changed items, and new bend
    // lines for later before we clear them in the aCommit. We'll need these to check for new
    // junctions needed, etc.
    SCH_SELECTION selectionCopy( aSelection );

    for( SCH_LINE* line : m_newDragLines )
        selectionCopy.Add( line );

    for( SCH_LINE* line : m_changedDragLines )
        selectionCopy.Add( line );

    lwbTool->TrimOverLappingWires( aCommit, &selectionCopy );
    lwbTool->AddJunctionsIfNeeded( aCommit, &selectionCopy );

    // This needs to run prior to `RecalculateConnections` because we need to identify the
    // lines that are newly dangling
    if( isDragLike && !isSlice )
        trimDanglingLines( aCommit );

    // Auto-rotate any moved labels
    for( EDA_ITEM* item : aSelection )
        m_frame->AutoRotateItem( m_frame->GetScreen(), static_cast<SCH_ITEM*>( item ) );

    m_frame->Schematic().CleanUp( aCommit );

    for( EDA_ITEM* item : m_frame->GetScreen()->Items() )
        item->ClearEditFlags();

    // Ensure any selected item not in screen main list (for instance symbol fields) has its
    // edit flags cleared
    for( EDA_ITEM* item : selectionCopy )
        item->ClearEditFlags();

    m_newDragLines.clear();
    m_changedDragLines.clear();

    if( aUnselect )
        m_toolMgr->RunAction( ACTIONS::selectionClear );
    else
        m_selectionTool->RebuildSelection();  // Schematic cleanup might have merged lines, etc.
}


void SCH_MOVE_TOOL::moveSelectionToSheet( SCH_SELECTION& aSelection, SCH_SHEET* aTargetSheet,
                                          SCH_COMMIT* aCommit )
{
    SCH_SCREEN* destScreen = aTargetSheet->GetScreen();
    SCH_SCREEN* srcScreen = m_frame->GetScreen();

    BOX2I bbox;

    for( EDA_ITEM* item : aSelection )
        bbox.Merge( static_cast<SCH_ITEM*>( item )->GetBoundingBox() );

    VECTOR2I offset = VECTOR2I( 0, 0 ) - bbox.GetPosition();
    int      step   = schIUScale.MilsToIU( 50 );
    bool     overlap = false;

    do
    {
        BOX2I moved = bbox;
        moved.Move( offset );
        overlap = false;

        for( SCH_ITEM* existing : destScreen->Items() )
        {
            if( moved.Intersects( existing->GetBoundingBox() ) )
            {
                overlap = true;
                break;
            }
        }

        if( overlap )
            offset += VECTOR2I( step, step );
    } while( overlap );

    for( EDA_ITEM* item : aSelection )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        // Remove from current screen and view manually
        m_frame->RemoveFromScreen( schItem, srcScreen );

        // Move the item
        schItem->Move( offset );

        // Add to destination screen manually (won't add to view since it's not current)
        destScreen->Append( schItem );

        // Record in commit with CHT_DONE flag to bypass automatic screen/view operations
        aCommit->Stage( schItem, CHT_REMOVE | CHT_DONE, srcScreen );
        aCommit->Stage( schItem, CHT_ADD | CHT_DONE, destScreen );
    }
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
        updateItem( line, false ); // Update any cached visuals before commit processes
        m_frame->RemoveFromScreen( line, m_frame->GetScreen() );
    }
}


void SCH_MOVE_TOOL::getConnectedItems( SCH_ITEM* aOriginalItem, const VECTOR2I& aPoint, EDA_ITEMS& aList )
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


void SCH_MOVE_TOOL::getConnectedDragItems( SCH_COMMIT* aCommit, SCH_ITEM* aSelectedItem, const VECTOR2I& aPoint,
                                           EDA_ITEMS& aList )
{
    EE_RTREE&              items = m_frame->GetScreen()->Items();
    EE_RTREE::EE_TYPE      itemsOverlappingRTree = items.Overlapping( aSelectedItem->GetBoundingBox() );
    std::vector<SCH_ITEM*> itemsConnectable;
    bool                   ptHasUnselectedJunction = false;

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

                SCH_LINE* selectedLine = dynamic_cast<SCH_LINE*>( selected );
                SCH_LINE* fixedLine = dynamic_cast<SCH_LINE*>( fixed );

                if( selectedLine )
                {
                    newWire->SetLastResolvedState( selected );
                    cloneWireConnection( newWire, selectedLine, m_frame );
                }
                else if( fixedLine )
                {
                    newWire->SetLastResolvedState( fixed );
                    cloneWireConnection( newWire, fixedLine, m_frame );
                }

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
                    && pin->GetPosition() == aPoint
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
            if( test->IsConnected( aPoint ) )
            {
                if( test->Type() == SCH_JUNCTION_T && test->GetPosition() == aPoint )
                {
                    test->SetFlags( SELECTED_BY_DRAG );
                    aList.push_back( test );
                }
                else if( !newWire )
                {
                    // Add a new wire between the symbol or junction and the selected item so
                    // the selected item can be dragged.
                    newWire = makeNewWire( aCommit, test, aSelectedItem, aPoint, aPoint );
                    newWire->SetFlags( SELECTED_BY_DRAG | STARTPOINT );
                    aList.push_back( newWire );
                }
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
    static int moveCallCount = 0;
    wxLogTrace( traceSchMove, "moveItem[%d]: type=%d, delta=(%d,%d)",
                ++moveCallCount, aItem->Type(), aDelta.x, aDelta.y );

    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

        if( aItem->HasFlag( STARTPOINT ) || ( m_mode == MOVE ) )
            line->MoveStart( aDelta );

        if( aItem->HasFlag( ENDPOINT ) || ( m_mode == MOVE ) )
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
                MOVE_MODE tmpMode = m_mode;
                m_mode = DRAG;
                moveItem( item, delta );
                m_mode = tmpMode;

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
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::breakWire.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::slice.MakeEvent() );
    Go( &SCH_MOVE_TOOL::AlignToGrid,        SCH_ACTIONS::alignToGrid.MakeEvent() );
}
