/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <bitmaps.h>
#include <eda_item.h>
#include <wx/log.h>
#include "symbol_editor_move_tool.h"
#include "symbol_editor_pin_tool.h"


SYMBOL_EDITOR_MOVE_TOOL::SYMBOL_EDITOR_MOVE_TOOL() :
        EE_TOOL_BASE( "eeschema.SymbolMoveTool" ),
        m_moveInProgress( false ),
        m_moveOffset( 0, 0 )
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
                        if( item->Type() != LIB_FIELD_T )
                            return false;
                    }
                }

                return true;
            };

    selToolMenu.AddItem( EE_ACTIONS::move, canMove && EE_CONDITIONS::IdleSelection, 150 );

    return true;
}


void SYMBOL_EDITOR_MOVE_TOOL::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD )
    {
        m_moveInProgress = false;
        m_moveOffset = { 0, 0 };
    }
}


int SYMBOL_EDITOR_MOVE_TOOL::Main( const TOOL_EVENT& aEvent )
{
    static KICAD_T fieldsOnly[] = { LIB_FIELD_T, EOT };

    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    m_anchorPos = { 0, 0 };

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    EE_SELECTION& selection = m_frame->IsSymbolAlias()
                                                ? m_selectionTool->RequestSelection( fieldsOnly )
                                                : m_selectionTool->RequestSelection();
    bool          unselect = selection.IsHover();

    if( !m_frame->IsSymbolEditable() || selection.Empty() || m_moveInProgress )
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool        restore_state = false;
    bool        chain_commands = false;
    TOOL_EVENT* evt = const_cast<TOOL_EVENT*>( &aEvent );
    VECTOR2I    prevPos;

    if( !selection.Front()->IsNew() )
        saveCopyInUndoList( m_frame->GetCurSymbol(), UNDO_REDO::LIBEDIT );

    m_cursor = controls->GetCursorPosition();

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );

        if( evt->IsAction( &EE_ACTIONS::move )
                || evt->IsMotion()
                || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview )
                || evt->IsAction( &EE_ACTIONS::symbolMoveActivate ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                LIB_ITEM* lib_item = static_cast<LIB_ITEM*>( selection.Front() );

                // Pick up any synchronized pins
                //
                // Careful when pasting.  The pasted pin will be at the same location as it
                // was copied from, leading us to believe it's a synchronized pin.  It's not.
                if( m_frame->SynchronizePins()
                        && ( lib_item->GetEditFlags() & IS_PASTED ) == 0 )
                {
                    std::set<LIB_PIN*> sync_pins;

                    for( EDA_ITEM* sel_item : selection )
                    {
                        lib_item = static_cast<LIB_ITEM*>( sel_item );

                        if(  lib_item->Type() == LIB_PIN_T )
                        {
                            LIB_PIN* cur_pin = static_cast<LIB_PIN*>( lib_item );
                            LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
                            std::vector<bool> got_unit( symbol->GetUnitCount() + 1 );

                            got_unit[cur_pin->GetUnit()] = true;

                            for( LIB_PIN* pin = symbol->GetNextPin(); pin;
                                 pin = symbol->GetNextPin( pin ) )
                            {
                                if( !got_unit[pin->GetUnit()]
                                 && pin->GetPosition() == cur_pin->GetPosition()
                                 && pin->GetOrientation() == cur_pin->GetOrientation()
                                 && pin->GetConvert() == cur_pin->GetConvert()
                                 && pin->GetType() == cur_pin->GetType()
                                 && pin->GetName() == cur_pin->GetName()  )
                                {
                                    if( sync_pins.insert( pin ).second )
                                        got_unit[pin->GetUnit()] = true;
                                }
                            }
                        }
                    }

                    for( LIB_PIN* pin : sync_pins )
                        m_selectionTool->AddItemToSel( pin, true /*quiet mode*/ );
                }

                // Apply any initial offset in case we're coming from a previous command.
                //
                for( EDA_ITEM* item : selection )
                    moveItem( item, m_moveOffset );

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( lib_item->IsNew() )
                {
                    m_anchorPos = selection.GetReferencePoint();
                    VECTOR2I delta = m_cursor - mapCoords( m_anchorPos );

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        moveItem( item, delta );
                        updateItem( item, false );
                    }

                    m_anchorPos = m_cursor;
                }
                else if( selection.Size() == 1 && m_frame->GetMoveWarpsCursor() )
                {
                    wxPoint itemPos = lib_item->GetPosition();
                    m_anchorPos = wxPoint( itemPos.x, -itemPos.y );

                    getViewControls()->WarpCursor( m_anchorPos, true, true );
                    m_cursor = m_anchorPos;
                }
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
            else
            {
                evt->SetPassEvent();
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
        else if( evt->IsMouseUp( BUT_LEFT )
                || evt->IsClick( BUT_LEFT )
                || evt->IsDblClick( BUT_LEFT ) )
        {
            if( selection.GetSize() == 1 && selection.Front()->Type() == LIB_PIN_T )
            {
                SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>();

                try
                {
                    if( !pinTool->PlacePin( (LIB_PIN*) selection.Front() ) )
                        restore_state = true;
                }
                catch( const boost::bad_pointer& e )
                {
                    restore_state = true;
                    wxLogError( "Boost pointer exception occurred: \"%s\"", e.what() );
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

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    m_anchorPos = { 0, 0 };

    for( EDA_ITEM* item : selection )
        item->ClearEditFlags();

    if( restore_state )
    {
        m_frame->RollbackSymbolFromUndo();

        if( unselect )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
        else
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
    else
    {
        if( unselect )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->OnModify();
    }

    m_moveInProgress = false;
    m_frame->PopTool( tool );
    return 0;
}


void SYMBOL_EDITOR_MOVE_TOOL::moveItem( EDA_ITEM* aItem, VECTOR2I aDelta )
{
    static_cast<LIB_ITEM*>( aItem )->Offset( mapCoords( aDelta ) );
    aItem->SetFlags( IS_MOVING );
}


void SYMBOL_EDITOR_MOVE_TOOL::setTransitions()
{
    Go( &SYMBOL_EDITOR_MOVE_TOOL::Main,               EE_ACTIONS::move.MakeEvent() );
    Go( &SYMBOL_EDITOR_MOVE_TOOL::Main,               EE_ACTIONS::symbolMoveActivate.MakeEvent() );
}
