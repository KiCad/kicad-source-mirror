/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <class_board.h>
#include <class_module.h>
#include <class_zone.h>
#include <wxPcbStruct.h>
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <ratsnest_data.h>
#include <confirm.h>

#include <cassert>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "common_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"

EDIT_TOOL::EDIT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveEdit" ), m_selectionTool( NULL )
{
}


bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    // Add context menu entries that are displayed when selection tool is active
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::editActivate );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::rotate );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::flip );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::remove );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::properties );

    setTransitions();

    return true;
}


int EDIT_TOOL::Main( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    // Be sure that there is at least one item that we can modify
    if( !makeSelection( selection ) )
    {
        setTransitions();

        return 0;
    }

    Activate();

    m_dragging = false;         // Are selected items being dragged?
    bool restore = false;       // Should items' state be restored when finishing the tool?

    // By default, modified items need to update their geometry
    m_updateFlag = KIGFX::VIEW_ITEM::GEOMETRY;

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );
    controls->ForceCursorPosition( false );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            restore = true; // Cancelling the tool means that items have to be restored
            break;          // Finish
        }

        else if( evt->Action() == TA_UNDO_REDO )
        {
            unselect = true;
            break;
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                Rotate( aEvent );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                Flip( aEvent );

                // Flip causes change of layers
                enableUpdateFlag( KIGFX::VIEW_ITEM::LAYERS );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::remove ) )
            {
                Remove( aEvent );

                break;       // exit the loop, as there is no further processing for removed items
            }
        }

        else if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            m_cursor = controls->GetCursorPosition();

            if( m_dragging )
            {
                wxPoint movement = wxPoint( m_cursor.x, m_cursor.y ) -
                                   selection.Item<BOARD_ITEM>( 0 )->GetPosition();

                // Drag items to the current cursor position
                for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
                    selection.Item<BOARD_ITEM>( i )->Move( movement + m_offset );

                updateRatsnest( true );
            }
            else    // Prepare to start dragging
            {
                // Save items, so changes can be undone
                editFrame->OnModify();
                editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

                if( evt->Modifier( MD_CTRL ) )
                {
                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    m_cursor = VECTOR2I( selection.Item<BOARD_ITEM>( 0 )->GetPosition() );
                    m_offset.x = 0;
                    m_offset.y = 0;
                }
                else
                {
                    // Update dragging offset (distance between cursor and the first dragged item)
                    m_offset = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) )->GetPosition() -
                                                         wxPoint( m_cursor.x, m_cursor.y );
                }

                m_dragging = true;
            }

            selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate );
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
            break; // Finish
    }

    m_dragging = false;

    if( restore )
    {
        // Modifications have to be rollbacked, so restore the previous state of items
        wxCommandEvent dummy;
        editFrame->RestoreCopyFromUndoList( dummy );
    }
    else
    {
        // Changes are applied, so update the items
        selection.group->ItemsViewUpdate( m_updateFlag );
    }

    if( unselect )
        m_selectionTool->ClearSelection();

    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();
    ratsnest->ClearSimple();
    ratsnest->Recalculate();

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Properties( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( !makeSelection( selection ) )
    {
        setTransitions();

        return 0;
    }

    // Properties are displayed when there is only one item selected
    if( selection.Size() == 1 )
    {
        // Display properties dialog
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( 0 );

        // Check if user wants to edit pad or module properties
        if( item->Type() == PCB_MODULE_T )
        {
            VECTOR2D cursor = getViewControls()->GetCursorPosition();

            for( D_PAD* pad = static_cast<MODULE*>( item )->Pads(); pad; pad = pad->Next() )
            {
                if( pad->ViewBBox().Contains( cursor ) )
                {
                    // Turns out that user wants to edit a pad properties
                    item = pad;
                    break;
                }
            }
        }

        std::vector<PICKED_ITEMS_LIST*>& undoList = editFrame->GetScreen()->m_UndoList.m_CommandsList;

        // Some of properties dialogs alter pointers, so we should deselect them
        m_selectionTool->ClearSelection();
        STATUS_FLAGS flags = item->GetFlags();
        item->ClearFlags();

        // It is necessary to determine if anything has changed
        PICKED_ITEMS_LIST* lastChange = undoList.empty() ? NULL : undoList.back();

        // Display properties dialog
        editFrame->OnEditItemRequest( NULL, item );

        PICKED_ITEMS_LIST* currentChange = undoList.empty() ? NULL : undoList.back();

        if( lastChange != currentChange )        // Something has changed
        {
            processChanges( currentChange );

            updateRatsnest( true );
            getModel<BOARD>()->GetRatsnest()->Recalculate();
            item->ViewUpdate();

            m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate );
        }

        item->SetFlags( flags );
    }

    setTransitions();

    return 0;
}


int EDIT_TOOL::Rotate( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !makeSelection( selection ) )
    {
        setTransitions();

        return 0;
    }

    wxPoint rotatePoint = getModificationPoint( selection );

    if( !m_dragging )   // If it is being dragged, then it is already saved with UR_CHANGED flag
    {
        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_ROTATED, rotatePoint );
    }

    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

        item->Rotate( rotatePoint, 900.0 /*m_frame->GetRotationAngle()*/ );

        if( !m_dragging )
            item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    updateRatsnest( m_dragging );

    // Update dragging offset (distance between cursor and the first dragged item)
    m_offset = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) )->GetPosition() -
                                         rotatePoint;

    if( m_dragging )
        selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    else
        getModel<BOARD>()->GetRatsnest()->Recalculate();

    if( unselect )
        m_selectionTool->ClearSelection();

    m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate );
    setTransitions();

    return 0;
}


int EDIT_TOOL::Flip( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !makeSelection( selection ) )
    {
        setTransitions();

        return 0;
    }

    wxPoint flipPoint = getModificationPoint( selection );

    if( !m_dragging )   // If it is being dragged, then it is already saved with UR_CHANGED flag
    {
        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_FLIPPED, flipPoint );
    }

    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

        item->Flip( flipPoint );

        if( !m_dragging )
            item->ViewUpdate( KIGFX::VIEW_ITEM::LAYERS );
    }

    updateRatsnest( m_dragging );

    // Update dragging offset (distance between cursor and the first dragged item)
    m_offset = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) )->GetPosition() -
                                         flipPoint;

    if( m_dragging )
        selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    else
        getModel<BOARD>()->GetRatsnest()->Recalculate();

    if( unselect )
        m_selectionTool->ClearSelection();

    m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate );
    setTransitions();

    return 0;
}


int EDIT_TOOL::Remove( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();

    if( !makeSelection( selection ) )
    {
        setTransitions();

        return 0;
    }

    // Get a copy of the selected items set
    PICKED_ITEMS_LIST selectedItems = selection.items;
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    // As we are about to remove items, they have to be removed from the selection first
    m_selectionTool->ClearSelection();

    // Save them
    for( unsigned int i = 0; i < selectedItems.GetCount(); ++i )
        selectedItems.SetPickedItemStatus( UR_DELETED, i );

    editFrame->OnModify();
    editFrame->SaveCopyInUndoList( selectedItems, UR_DELETED );

    // And now remove
    for( unsigned int i = 0; i < selectedItems.GetCount(); ++i )
        remove( static_cast<BOARD_ITEM*>( selectedItems.GetPickedItem( i ) ) );

    getModel<BOARD>()->GetRatsnest()->Recalculate();

    setTransitions();

    return 0;
}


void EDIT_TOOL::remove( BOARD_ITEM* aItem )
{
    BOARD* board = getModel<BOARD>();

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
    {
        MODULE* module = static_cast<MODULE*>( aItem );
        module->ClearFlags();
        module->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, getView(), _1 ) );

        // Module itself is deleted after the switch scope is finished
        // list of pads is rebuild by BOARD::BuildListOfNets()

        // Clear flags to indicate, that the ratsnest, list of nets & pads are not valid anymore
        board->m_Status_Pcb = 0;
    }
    break;

    // These are not supposed to be removed
    case PCB_PAD_T:
    case PCB_MODULE_TEXT_T:
    case PCB_MODULE_EDGE_T:
        return;

    case PCB_LINE_T:                // a segment not on copper layers
    case PCB_TEXT_T:                // a text on a layer
    case PCB_TRACE_T:               // a track segment (segment on a copper layer)
    case PCB_VIA_T:                 // a via (like track segment on a copper layer)
    case PCB_DIMENSION_T:           // a dimension (graphic item)
    case PCB_TARGET_T:              // a target (graphic item)
    case PCB_MARKER_T:              // a marker used to show something
    case PCB_ZONE_T:                // SEG_ZONE items are now deprecated
    case PCB_ZONE_AREA_T:
        break;

    default:                        // other types do not need to (or should not) be handled
        assert( false );
        return;
        break;
    }

    getView()->Remove( aItem );
    board->Remove( aItem );
}


void EDIT_TOOL::setTransitions()
{
    Go( &EDIT_TOOL::Main,       COMMON_ACTIONS::editActivate.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,     COMMON_ACTIONS::rotate.MakeEvent() );
    Go( &EDIT_TOOL::Flip,       COMMON_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,     COMMON_ACTIONS::remove.MakeEvent() );
    Go( &EDIT_TOOL::Properties, COMMON_ACTIONS::properties.MakeEvent() );
}


void EDIT_TOOL::updateRatsnest( bool aRedraw )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

    ratsnest->ClearSimple();
    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

        ratsnest->Update( item );

        if( aRedraw )
            ratsnest->AddSimple( item );
    }
}


wxPoint EDIT_TOOL::getModificationPoint( const SELECTION_TOOL::SELECTION& aSelection )
{
    if( aSelection.Size() == 1 )
    {
        return aSelection.Item<BOARD_ITEM>( 0 )->GetPosition() - m_offset;
    }
    else
    {
        // If EDIT_TOOL is not currently active then it means that the cursor position is not
        // updated, so we have to fetch the latest value
        if( m_toolMgr->GetCurrentToolId() != m_toolId )
            m_cursor = getViewControls()->GetCursorPosition();

        return wxPoint( m_cursor.x, m_cursor.y );
    }
}


bool EDIT_TOOL::makeSelection( const SELECTION_TOOL::SELECTION& aSelection )
{
    if( aSelection.Empty() )                        // Try to find an item that could be modified
        m_selectionTool->SelectSingle( getView()->ToWorld( getViewControls()->GetMousePosition() ) );

    return !aSelection.Empty();
}


void EDIT_TOOL::processChanges( const PICKED_ITEMS_LIST* aList )
{
    for( unsigned int i = 0; i < aList->GetCount(); ++i )
    {
        UNDO_REDO_T operation = aList->GetPickedItemStatus( i );
        EDA_ITEM* updItem = aList->GetPickedItem( i );

        switch( operation )
        {
        case UR_CHANGED:
        case UR_MODEDIT:
            updItem->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;

        case UR_DELETED:
            if( updItem->Type() == PCB_MODULE_T )
                static_cast<MODULE*>( updItem )->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove,
                                                                             getView(), _1 ) );

            getView()->Remove( updItem );
            break;

        case UR_NEW:
            if( updItem->Type() == PCB_MODULE_T )
                static_cast<MODULE*>( updItem )->RunOnChildren( boost::bind( &KIGFX::VIEW::Add,
                                                                             getView(), _1 ) );

            getView()->Add( updItem );
            updItem->ViewUpdate();
            break;

        default:
            assert( false );    // Not handled
            break;
        }
    }
}
