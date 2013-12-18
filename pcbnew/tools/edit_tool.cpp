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
#include <confirm.h>
#include <cassert>

#include "common_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"

using namespace KIGFX;
using boost::optional;

EDIT_TOOL::EDIT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveEdit" ), m_selectionTool( NULL )
{
}


bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    TOOL_BASE* selectionTool = m_toolMgr->FindTool( "pcbnew.InteractiveSelection" );

    m_selectionTool = static_cast<SELECTION_TOOL*>( selectionTool );
    if( !selectionTool )
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
    PCB_EDIT_FRAME* editFrame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );

    // By default, modified items need to update their geometry
    m_updateFlag = KIGFX::VIEW_ITEM::GEOMETRY;

    if( selection.Empty() )
        return 0; // there are no items to operate on

    VECTOR2D dragPosition;      // The last position of the cursor while dragging
    m_dragging = false;
    bool restore = false;       // Should items' state be restored when finishing the tool?

    VIEW_CONTROLS* controls = getViewControls();
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            restore = true; // Cancelling the tool means that items have to be restored
            break;          // Finish
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
        }

        else if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            if( m_dragging )
            {
                // Drag items to the current cursor position
                VECTOR2D movement = ( evt->Position() - dragPosition );
                for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
                {
                    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( i ) );
                    item->Move( wxPoint( movement.x, movement.y ) );
                }
            }
            else
            {
                // Prepare to drag - save items, so changes can be undone
                editFrame->OnModify();
                editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

                m_dragging = true;
            }

            selection.group->ViewUpdate( VIEW_ITEM::GEOMETRY );
            dragPosition = evt->Position();
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
            break; // Finish
    }

    m_dragging = false;

    if( restore )
    {
        // Modifications has to be rollbacked, so restore the previous state of items
        wxCommandEvent dummy;
        editFrame->GetBoardFromUndoList( dummy );
    }
    else
    {
        // Changes are applied, so update the items
        selection.group->ItemsViewUpdate( m_updateFlag );
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Properties( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    PCB_EDIT_FRAME* editFrame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );

    // Properties are displayed when there is only one item selected
    if( selection.Size() == 1 )
    {
        // Display properties dialog
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) );

        if( !m_dragging )   // If it is being dragged, then it is already saved with UR_CHANGED flag
        {
            editFrame->SaveCopyInUndoList( item, UR_CHANGED );
            editFrame->OnModify();
        }

        editFrame->OnEditItemRequest( NULL, item );

        item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    setTransitions();

    return 0;
}


int EDIT_TOOL::Rotate( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    VECTOR2D cursor = getView()->ToWorld( getViewControls()->GetCursorPosition() );
    PCB_EDIT_FRAME* editFrame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );

    if( !m_dragging )   // If it is being dragged, then it is already saved with UR_CHANGED flag
    {
        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_ROTATED, wxPoint( cursor.x, cursor.y ) );
    }

    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( i ) );

        item->Rotate( wxPoint( cursor.x, cursor.y ), 900.0 );
        if( !m_dragging )
            item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    if( m_dragging )
        selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Flip( TOOL_EVENT& aEvent )
{
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();
    VECTOR2D cursor = getView()->ToWorld( getViewControls()->GetCursorPosition() );
    PCB_EDIT_FRAME* editFrame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );

    if( !m_dragging )   // If it is being dragged, then it is already saved with UR_CHANGED flag
    {
        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_FLIPPED, wxPoint( cursor.x, cursor.y ) );
    }

    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( i ) );

        item->Flip( wxPoint( cursor.x, cursor.y ) );
        if( !m_dragging )
            item->ViewUpdate( KIGFX::VIEW_ITEM::LAYERS );
    }

    if( m_dragging )
        selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Remove( TOOL_EVENT& aEvent )
{
    // Get a copy of the selected items set
    PICKED_ITEMS_LIST selectedItems = m_selectionTool->GetSelection().items;
    PCB_EDIT_FRAME* editFrame = static_cast<PCB_EDIT_FRAME*>( m_toolMgr->GetEditFrame() );

    // As we are about to remove items, they have to be removed from the selection first
    m_selectionTool->ClearSelection();

    // Save them
    for( unsigned int i = 0; i < selectedItems.GetCount(); ++i )
        selectedItems.SetPickedItemStatus( UR_DELETED, i );
    editFrame->OnModify();
    editFrame->SaveCopyInUndoList( selectedItems, UR_DELETED );

    // And now remove
    for( unsigned int i = 0; i < selectedItems.GetCount(); ++i )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selectedItems.GetPickedItem( i ) );
        remove( item );
    }

    // Rebuild list of pads and nets if necessary
    BOARD* board = getModel<BOARD>( PCB_T );
    if( !( board->GetStatus() & NET_CODES_OK ) )
        board->BuildListOfNets();

    setTransitions();

    return 0;
}


void EDIT_TOOL::remove( BOARD_ITEM* aItem )
{
    BOARD* board = getModel<BOARD>( PCB_T );

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
    {
        MODULE* module = static_cast<MODULE*>( aItem );
        module->ClearFlags();

        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
            getView()->Remove( pad );

        for( BOARD_ITEM* drawing = module->GraphicalItems().GetFirst(); drawing;
             drawing = drawing->Next() )
            getView()->Remove( drawing );

        getView()->Remove( &module->Reference() );
        getView()->Remove( &module->Value() );

        // Module itself is deleted after the switch scope is finished
        // list of pads is rebuild by BOARD::BuildListOfNets()

        // Clear flags to indicate, that the ratsnest, list of nets & pads are not valid anymore
        board->m_Status_Pcb = 0;
    }
    break;

    case PCB_ZONE_AREA_T:
        getView()->Remove( aItem );
        getModel<BOARD>( PCB_T )->Delete( aItem );
        return;

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
