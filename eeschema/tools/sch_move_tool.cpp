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

#include <tool/tool_manager.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_drawing_tool.h>
#include <tools/sch_line_drawing_tool.h>
#include <tools/sch_picker_tool.h>
#include <sch_actions.h>
#include <hotkeys.h>
#include <bitmaps.h>
#include <base_struct.h>
#include <sch_item_struct.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include "sch_move_tool.h"


TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drag( "eeschema.InteractiveEdit.drag",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DRAG ),
        _( "Drag" ), _( "Drags the selected item(s)" ), move_xpm, AF_ACTIVATE );


// For adding to or removing from selections
#define QUIET_MODE true


SCH_MOVE_TOOL::SCH_MOVE_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveMove" ),
        m_selectionTool( nullptr ),
        m_controls( nullptr ),
        m_frame( nullptr ),
        m_menu( *this ),
        m_moveInProgress( false ),
        m_moveOffset( 0, 0 )
{
}


SCH_MOVE_TOOL::~SCH_MOVE_TOOL()
{
}


bool SCH_MOVE_TOOL::Init()
{
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );

    auto activeTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto moveCondition = [] ( const SELECTION& aSel ) {
        if( aSel.Empty() )
            return false;

        if( SCH_LINE_DRAWING_TOOL::IsDrawingLineWireOrBus( aSel ) )
            return false;

        return true;
    };

    //
    // Build the tool menu
    //
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeTool, 1 );

    ctxMenu.AddSeparator( SELECTION_CONDITIONS::NotEmpty, 1000 );
    m_menu.AddStandardSubMenus( m_frame );

    //
    // Add move actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::move, moveCondition, 150 );
    selToolMenu.AddItem( SCH_ACTIONS::drag, moveCondition, 150 );

    return true;
}


void SCH_MOVE_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        m_moveInProgress = false;
        m_moveOffset = { 0, 0 };

        // Init variables used by every drawing tool
        m_controls = getViewControls();
        m_frame = getEditFrame<SCH_EDIT_FRAME>();
    }
}


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
    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    SELECTION& selection = m_selectionTool->RequestSelection( movableItems );
    EDA_ITEMS  dragAdditions;
    bool       unselect = selection.IsHover();

    if( selection.Empty() )
        return 0;

    if( aEvent.IsAction( &SCH_ACTIONS::move ) )
        m_frame->SetToolID( ID_SCH_MOVE, wxCURSOR_DEFAULT, _( "Move Items" ) );
    else
        m_frame->SetToolID( ID_SCH_DRAG, wxCURSOR_DEFAULT, _( "Drag Items" ) );

    Activate();
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool restore_state = false;
    bool chain_commands = false;
    OPT_TOOL_EVENT evt = aEvent;
    VECTOR2I prevPos;

    if( m_moveInProgress )
    {
        // User must have switched from move to drag or vice-versa.  Reset the moved items
        // so we can start again with the current m_isDragOperation and m_moveOffset.
        m_frame->RollbackSchematicFromUndo();
        m_selectionTool->RemoveItemsFromSel( &dragAdditions, QUIET_MODE );
        m_moveInProgress = false;
        // And give it a kick so it doesn't have to wait for the first mouse movement to
        // refresh.
        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
        return 0;
    }

    // Main loop: keep receiving events
    do
    {
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &SCH_ACTIONS::move ) || evt->IsAction( &SCH_ACTIONS::drag )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &SCH_ACTIONS::refreshPreview ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                //------------------------------------------------------------------------
                // Setup a drag or a move
                //
                for( SCH_ITEM* it = m_frame->GetScreen()->GetDrawItems(); it; it = it->Next() )
                {
                    if( it->IsSelected() )
                        it->SetFlags( STARTPOINT | ENDPOINT | SELECTEDNODE );
                    else
                        it->ClearFlags( STARTPOINT | ENDPOINT | SELECTEDNODE );
                }

                // Add connections to the selection for a drag.
                //
                if( m_frame->GetToolId() == ID_SCH_DRAG )
                {
                    for( EDA_ITEM* item : selection )
                    {
                        if( static_cast<SCH_ITEM*>( item )->IsConnectable() )
                        {
                            std::vector<wxPoint> connections;
                            static_cast<SCH_ITEM*>( item )->GetConnectionPoints( connections );

                            for( wxPoint point : connections )
                                getConnectedDragItems( (SCH_ITEM*) item, point, dragAdditions );
                        }
                    }

                    m_selectionTool->AddItemsToSel( &dragAdditions, QUIET_MODE );

                    for( EDA_ITEM* item : dragAdditions )
                        saveCopyInUndoList( (SCH_ITEM*) item, UR_CHANGED, true );
                }

                // Mark the edges of the block with dangling flags for a move.
                //
                if( m_frame->GetToolId() == ID_SCH_MOVE )
                {
                    std::vector<DANGLING_END_ITEM> internalPoints;

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->GetEndPoints( internalPoints );

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( internalPoints );
                }

                // Generic setup
                //
                bool first = true;
                for( EDA_ITEM* item : selection )
                {
                    if( item->IsNew() || ( item->GetParent() && item->GetParent()->IsSelected() ) )
                    {
                        // already saved to undo
                    }
                    else
                    {
                        saveCopyInUndoList( (SCH_ITEM*) item, UR_CHANGED, !first );
                        first = false;
                    }

                    // Apply any initial offset in case we're coming from a previous command.
                    //
                    moveItem( (SCH_ITEM*) item, m_moveOffset, m_frame->GetToolId() == ID_SCH_DRAG );
                }

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta, m_frame->GetToolId() == ID_SCH_DRAG );
                        updateView( item );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin,
                    // so the movement vector can be computed later
                    updateModificationPoint( selection );
                    m_cursor = originalCursorPos;
                }
                else
                {
                    updateModificationPoint( selection );
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = controls->GetCursorPosition();
            VECTOR2I delta( m_cursor - prevPos );
            selection.SetReferencePoint( m_cursor );

            m_moveOffset += delta;
            prevPos = m_cursor;

            for( EDA_ITEM* item : selection )
            {
                // Don't double move pins, fields, etc.
                if( item->GetParent() && item->GetParent()->IsSelected() )
                    continue;

                moveItem( (SCH_ITEM*) item, delta, m_frame->GetToolId() == ID_SCH_DRAG );
                updateView( item );
            }

            m_frame->UpdateMsgPanel();
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

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
            if( evt->IsAction( &SCH_ACTIONS::doDelete ) )
            {
                // Exit on a remove operation; there is no further processing for removed items.
                break;
            }
            else if( evt->IsAction( &SCH_ACTIONS::duplicate ) )
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
            else if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
            {
                if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                    && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_CMP_MAX )
                {
                    SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( selection.Front() );
                    int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                    if( component )
                    {
                        m_frame->SelectUnit( component, unit );
                        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
                    }
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    m_moveInProgress = false;
    m_frame->SetNoToolSelected();

    selection.ClearReferencePoint();

    for( auto item : selection )
        item->ClearFlags( IS_MOVED );

    if( unselect )
        m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    else
        m_selectionTool->RemoveItemsFromSel( &dragAdditions, QUIET_MODE );

    if( restore_state )
    {
        m_frame->RollbackSchematicFromUndo();
    }
    else
    {
        m_frame->CheckConnections( selection, true );
        m_frame->SchematicCleanUp( true );
        m_frame->TestDanglingEnds();
        m_frame->OnModify();
    }

    return 0;
}


void SCH_MOVE_TOOL::getConnectedDragItems( SCH_ITEM* aItem, wxPoint aPoint, EDA_ITEMS& aList )
{
    for( SCH_ITEM* test = m_frame->GetScreen()->GetDrawItems(); test; test = test->Next() )
    {
        if( test->IsSelected() || !test->IsConnectable() || !test->CanConnect( aItem ) )
            continue;

        switch( test->Type() )
        {
        default:
        case SCH_LINE_T:
        {
            // Select wires/busses that are connected at one end and/or the other.  Any
            // unconnected ends must be flagged (STARTPOINT or ENDPOINT).
            SCH_LINE* line = (SCH_LINE*) test;

            if( line->GetStartPoint() == aPoint )
            {
                if( !( line->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( line );

                line->SetFlags( STARTPOINT | SELECTEDNODE );
            }
            else if( line->GetEndPoint() == aPoint )
            {
                if( !( line->GetFlags() & SELECTEDNODE ) )
                    aList.push_back( line );

                line->SetFlags( ENDPOINT | SELECTEDNODE );
            }
            break;
        }

        case SCH_SHEET_T:
            // Dragging a sheet just because it's connected to something else feels a bit like
            // the tail wagging the dog, but this could be moved down to the next case.
            break;

        case SCH_COMPONENT_T:
        case SCH_NO_CONNECT_T:
        case SCH_JUNCTION_T:
            // Select connected items that have no wire between them.
            if( aItem->Type() != SCH_LINE_T && test->IsConnected( aPoint ) )
               aList.push_back( test );

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Select labels and bus entries that are connected to a wire being moved.
            if( aItem->Type() == SCH_LINE_T )
            {
                std::vector<wxPoint> connections;
                test->GetConnectionPoints( connections );

                for( wxPoint& point : connections )
                {
                    if( aItem->HitTest( point ) )
                        aList.push_back( test );
                }
            }
            break;
        }
    }
}


void SCH_MOVE_TOOL::moveItem( SCH_ITEM* aItem, VECTOR2I aDelta, bool isDrag )
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
        aItem->Move( wxPoint( aDelta.x, -aDelta.y ) );
        break;

    default:
        aItem->Move( (wxPoint) aDelta );
        break;
    }

    aItem->SetFlags( IS_MOVED );
}


bool SCH_MOVE_TOOL::updateModificationPoint( SELECTION& aSelection )
{
    if( m_moveInProgress && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        SCH_ITEM* item =  static_cast<SCH_ITEM*>( aSelection.Front() );

        // For some items, moving the cursor to anchor is not good (for instance large
        // hierarchical sheets or components can have the anchor outside the view)
        if( item->IsMovableFromAnchorPoint() )
        {
            wxPoint pos = item->GetPosition();
            aSelection.SetReferencePoint( pos );

            return true;
        }
    }

    // ...otherwise modify items with regard to the grid-snapped cursor position
    m_cursor = getViewControls()->GetCursorPosition( true );
    aSelection.SetReferencePoint( m_cursor );

    return true;
}


void SCH_MOVE_TOOL::updateView( EDA_ITEM* aItem )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );

    getView()->Update( aItem );
}


void SCH_MOVE_TOOL::saveCopyInUndoList( SCH_ITEM* aItem, UNDO_REDO_T aType, bool aAppend )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        m_frame->SaveCopyInUndoList( (SCH_ITEM*)aItem->GetParent(), UR_CHANGED, aAppend );
    else
        m_frame->SaveCopyInUndoList( aItem, aType, aAppend );
}


void SCH_MOVE_TOOL::setTransitions()
{
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::move.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               SCH_ACTIONS::drag.MakeEvent() );
}
