/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <ee_actions.h>
#include <ee_grid_helper.h>
#include <eda_item.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_shape.h>
#include <sch_commit.h>
#include <wx/debug.h>
#include <view/view_controls.h>
#include "symbol_editor_move_tool.h"
#include "symbol_editor_pin_tool.h"


SYMBOL_EDITOR_MOVE_TOOL::SYMBOL_EDITOR_MOVE_TOOL() :
        EE_TOOL_BASE( "eeschema.SymbolMoveTool" ),
        m_moveInProgress( false )
{
}


bool SYMBOL_EDITOR_MOVE_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    //
    // Add move actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    auto canMove =
            [&]( const SELECTION& sel )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
                wxCHECK( editor, false );

                if( !editor->IsSymbolEditable() )
                    return false;

                if( editor->IsSymbolAlias() )
                {
                    for( EDA_ITEM* item : sel )
                    {
                        if( item->Type() != SCH_FIELD_T )
                            return false;
                    }
                }

                return true;
            };

    selToolMenu.AddItem( EE_ACTIONS::move,        canMove && EE_CONDITIONS::IdleSelection, 150 );
    selToolMenu.AddItem( EE_ACTIONS::alignToGrid, canMove && EE_CONDITIONS::IdleSelection, 150 );

    return true;
}


void SYMBOL_EDITOR_MOVE_TOOL::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD )
        m_moveInProgress = false;
}


int SYMBOL_EDITOR_MOVE_TOOL::Main( const TOOL_EVENT& aEvent )
{
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
            localCommit.Push( _( "Move" ) );
        else
            localCommit.Revert();
    }

    return 0;
}


bool SYMBOL_EDITOR_MOVE_TOOL::doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );

    m_anchorPos = { 0, 0 };

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    EE_SELECTION& selection = m_frame->IsSymbolAlias()
                                            ? m_selectionTool->RequestSelection( { SCH_FIELD_T } )
                                            : m_selectionTool->RequestSelection();
    bool          unselect = selection.IsHover();

    if( !m_frame->IsSymbolEditable() || selection.Empty() )
        return false;

    if( m_moveInProgress )
    {
        // The tool hotkey is interpreted as a click when already moving
        m_toolMgr->RunAction( ACTIONS::cursorClick );
        return true;
    }

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool        restore_state = false;
    TOOL_EVENT  copy = aEvent;
    TOOL_EVENT* evt = &copy;
    VECTOR2I    prevPos;
    VECTOR2I    moveOffset;

    if( !selection.Front()->IsNew() )
        aCommit->Modify( m_frame->GetCurSymbol(), m_frame->GetScreen() );

    m_cursor = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( evt->IsAction( &EE_ACTIONS::move )
                || evt->IsMotion()
                || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            GRID_HELPER_GRIDS snapLayer = grid.GetSelectionGrid( selection );

            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                SCH_ITEM* lib_item = static_cast<SCH_ITEM*>( selection.Front() );

                // Pick up any synchronized pins
                //
                // Careful when pasting.  The pasted pin will be at the same location as it
                // was copied from, leading us to believe it's a synchronized pin.  It's not.
                if( m_frame->SynchronizePins() && !( lib_item->GetEditFlags() & IS_PASTED ) )
                {
                    std::set<SCH_PIN*> sync_pins;

                    for( EDA_ITEM* sel_item : selection )
                    {
                        lib_item = static_cast<SCH_ITEM*>( sel_item );

                        if(  lib_item->Type() == SCH_PIN_T )
                        {
                            SCH_PIN*          cur_pin = static_cast<SCH_PIN*>( lib_item );
                            LIB_SYMBOL*       symbol = m_frame->GetCurSymbol();
                            std::vector<bool> got_unit( symbol->GetUnitCount() + 1 );

                            got_unit[cur_pin->GetUnit()] = true;

                            for( SCH_PIN* pin : symbol->GetPins() )
                            {
                                if( !got_unit[pin->GetUnit()]
                                        && pin->GetPosition() == cur_pin->GetPosition()
                                        && pin->GetOrientation() == cur_pin->GetOrientation()
                                        && pin->GetBodyStyle() == cur_pin->GetBodyStyle()
                                        && pin->GetType() == cur_pin->GetType()
                                        && pin->GetName() == cur_pin->GetName()  )
                                {
                                    if( sync_pins.insert( pin ).second )
                                        got_unit[pin->GetUnit()] = true;
                                }
                            }
                        }
                    }

                    for( SCH_PIN* pin : sync_pins )
                        m_selectionTool->AddItemToSel( pin, true /*quiet mode*/ );
                }

                // Apply any initial offset in case we're coming from a previous command.
                //
                for( EDA_ITEM* item : selection )
                    moveItem( item, moveOffset );

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition( !evt->DisableGridSnapping() );

                if( lib_item->IsNew() )
                {
                    m_anchorPos = selection.GetReferencePoint();
                    VECTOR2I delta = m_cursor - m_anchorPos;

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        moveItem( item, delta );
                        updateItem( item, false );
                    }

                    m_anchorPos = m_cursor;
                }
                else if( m_frame->GetMoveWarpsCursor() )
                {
                    // User wants to warp the mouse
                    m_cursor = grid.BestDragOrigin( m_cursor, snapLayer, selection );
                    selection.SetReferencePoint( m_cursor );
                    m_anchorPos = m_cursor;
                }
                else
                {
                    m_cursor = controls->GetCursorPosition( !evt->DisableGridSnapping() );
                    m_anchorPos = m_cursor;
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = grid.BestSnapAnchor( controls->GetCursorPosition( false ), snapLayer,
                                            selection );
            VECTOR2I delta( m_cursor - prevPos );
            m_anchorPos = m_cursor;

            moveOffset += delta;
            prevPos = m_cursor;

            for( EDA_ITEM* item : selection )
            {
                moveItem( item, delta );
                updateItem( item, false );
            }

            m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_moveInProgress )
            {
                evt->SetPassEvent( false );
                restore_state = true;
            }

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
        else if( evt->IsAction( &ACTIONS::doDelete ) )
        {
            // Exit on a remove operation; there is no further processing for removed items.
            break;
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) )
        {
            wxBell();
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
            if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_PIN_T )
            {
                SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>();

                try
                {
                    SCH_PIN* curr_pin = static_cast<SCH_PIN*>( selection.Front() );

                    if( pinTool->PlacePin( curr_pin ) )
                    {
                        // PlacePin() clears the current selection, which we don't want.  Not only
                        // is it a poor user experience, but it also prevents us from doing the
                        // proper cleanup at the end of this routine (ie: clearing the edit flags).
                        m_selectionTool->AddItemToSel( curr_pin, true /*quiet mode*/ );
                    }
                    else
                    {
                        restore_state = true;
                    }
                }
                catch( const boost::bad_pointer& e )
                {
                    restore_state = true;
                    wxFAIL_MSG( wxString::Format( wxT( "Boost pointer exception occurred: %s" ),
                                                  e.what() ) );
                }
            }

            break; // Finish
        }
        else
        {
            evt->SetPassEvent();
        }

    } while( ( evt = Wait() ) );  // Assignment intentional; not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_anchorPos = { 0, 0 };

    for( EDA_ITEM* item : selection )
        item->ClearEditFlags();

    if( unselect )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    m_moveInProgress = false;
    m_frame->PopTool( aEvent );

    return !restore_state;
}


int SYMBOL_EDITOR_MOVE_TOOL::AlignElements( const TOOL_EVENT& aEvent )
{
    EE_GRID_HELPER    grid( m_toolMgr);
    EE_SELECTION&     selection = m_selectionTool->RequestSelection();
    SCH_COMMIT        commit( m_toolMgr );

    auto doMoveItem =
            [&]( EDA_ITEM* item, const VECTOR2I& delta )
            {
                commit.Modify( item, m_frame->GetScreen() );
                static_cast<SCH_ITEM*>( item )->Move( delta );
                updateItem( item, true );
            };

    for( EDA_ITEM* item : selection )
    {
        if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( item ) )
        {
            VECTOR2I newStart = grid.AlignGrid( shape->GetStart(), grid.GetItemGrid( shape ) );
            VECTOR2I newEnd = grid.AlignGrid( shape->GetEnd(), grid.GetItemGrid( shape ) );

            switch( shape->GetShape() )
            {
            case SHAPE_T::SEGMENT:
            case SHAPE_T::RECTANGLE:
            case SHAPE_T::CIRCLE:
            case SHAPE_T::ARC:
                if( newStart == newEnd ||
                    shape->GetShape() == SHAPE_T::CIRCLE || shape->GetShape() == SHAPE_T::ARC )
                {
                    // For arc and circle, never modify the shape. just snap its position
                    // For others, don't collapse shape; just snap its position
                    if( newStart != shape->GetStart() )
                        doMoveItem( shape, newStart - shape->GetStart() );
                }
                else if( newStart != shape->GetStart() || newEnd != shape->GetEnd() )
                {
                    // Snap both ends
                    commit.Modify( shape, m_frame->GetScreen() );

                    shape->SetStart( newStart );
                    shape->SetEnd( newEnd );

                    updateItem( item, true );
                }

                break;

            case SHAPE_T::POLY:
                if( shape->GetPointCount() > 0 )
                {
                    std::vector<VECTOR2I> newPts;

                    for( const VECTOR2I& pt : shape->GetPolyShape().Outline( 0 ).CPoints() )
                        newPts.push_back( grid.AlignGrid( pt, grid.GetItemGrid( shape ) ) );

                    bool collapsed = false;

                    for( int ii = 0; ii < (int) newPts.size() - 1; ++ii )
                    {
                        if( newPts[ii] == newPts[ii + 1] )
                            collapsed = true;
                    }

                    if( collapsed )
                    {
                        // Don't collapse shape; just snap its position
                        if( newStart != shape->GetStart() )
                            doMoveItem( shape, newStart - shape->GetStart() );
                    }
                    else
                    {
                        commit.Modify( shape, m_frame->GetScreen() );

                        for( int ii = 0; ii < (int) newPts.size(); ++ii )
                            shape->GetPolyShape().Outline( 0 ).SetPoint( ii, newPts[ii] );

                        updateItem( item, true );
                    }
                }

                break;

            case SHAPE_T::BEZIER:
                // Snapping bezier control points is unlikely to be useful.  Just snap its
                // position.
                if( newStart != shape->GetStart() )
                    doMoveItem( shape, newStart - shape->GetStart() );

                break;

            case SHAPE_T::UNDEFINED:
                wxASSERT_MSG( false, wxT( "Undefined shape in AlignElements" ) );
                break;
            }
        }
        else
        {
            VECTOR2I newPos = grid.AlignGrid( item->GetPosition(), grid.GetItemGrid( item ) );
            VECTOR2I delta = newPos - item->GetPosition();

            if( delta != VECTOR2I( 0, 0 ) )
                doMoveItem( item, delta );

            if( SCH_PIN* pin = dynamic_cast<SCH_PIN*>( item ) )
            {
                int length = pin->GetLength();
                int pinGrid;

                if( pin->GetOrientation() == PIN_ORIENTATION::PIN_LEFT
                        || pin->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
                {
                    pinGrid = KiROUND( grid.GetGridSize( grid.GetItemGrid( item ) ).x );
                }
                else
                {
                    pinGrid = KiROUND( grid.GetGridSize( grid.GetItemGrid( item ) ).y );
                }

                int newLength = KiROUND( (double) length / pinGrid ) * pinGrid;

                if( newLength > 0 )
                    pin->SetLength( newLength );
            }
        }
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );

    commit.Push( _( "Align" ) );
    return 0;
}


void SYMBOL_EDITOR_MOVE_TOOL::moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta )
{
    static_cast<SCH_ITEM*>( aItem )->Move( aDelta );
    aItem->SetFlags( IS_MOVING );
}


void SYMBOL_EDITOR_MOVE_TOOL::setTransitions()
{
    Go( &SYMBOL_EDITOR_MOVE_TOOL::Main,               EE_ACTIONS::move.MakeEvent() );
    Go( &SYMBOL_EDITOR_MOVE_TOOL::AlignElements,      EE_ACTIONS::alignToGrid.MakeEvent() );
}
