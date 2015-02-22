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
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_draw_panel_gal.h>
#include <module_editor_frame.h>
#include <wxPcbStruct.h>

#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <ratsnest_data.h>
#include <confirm.h>

#include <cassert>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "common_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"

#include <dialogs/dialog_create_array.h>
#include <dialogs/dialog_move_exact.h>

EDIT_TOOL::EDIT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveEdit" ), m_selectionTool( NULL ),
    m_dragging( false ), m_editModules( false ), m_undoInhibit( 0 ),
    m_updateFlag( KIGFX::VIEW_ITEM::NONE )
{
}


void EDIT_TOOL::Reset( RESET_REASON aReason )
{
    m_dragging = false;
    m_updateFlag = KIGFX::VIEW_ITEM::NONE;
}


bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    // Add context menu entries that are displayed when selection tool is active
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::editActivate, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::rotate, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::flip, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::remove, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::properties, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::moveExact, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::duplicate, SELECTION_CONDITIONS::NotEmpty );
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::createArray, SELECTION_CONDITIONS::NotEmpty );

    m_offset.x = 0;
    m_offset.y = 0;

    setTransitions();

    return true;
}


int EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

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
    controls->ForceCursorPosition( false );

    // cumulative translation
    wxPoint totalMovement( 0, 0 );

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
            else if( evt->IsAction( &COMMON_ACTIONS::duplicate ) )
            {
                // On duplicate, stop moving this item
                // The duplicate tool should then select the new item and start
                // a new move procedure
                break;
            }
            else if( evt->IsAction( &COMMON_ACTIONS::moveExact ) )
            {
                // Can't do this, because the selection will then contain
                // stale pointers and it will all go horribly wrong...
                //editFrame->RestoreCopyFromUndoList( dummy );
                //
                // So, instead, reset the position manually
                for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
                {
                    BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );
                    item->SetPosition( item->GetPosition() - totalMovement );

                    // And what about flipping and rotation?
                    // for now, they won't be undone, but maybe that is how
                    // it should be, so you can flip and move exact in the
                    // same action?
                }

                // This causes a double event, so we will get the dialogue
                // correctly, somehow - why does Rotate not?
                //MoveExact( aEvent );
                break;      // exit the loop - we move exactly, so we have
                            // finished moving
            }
        }

        else if( evt->IsAction( &COMMON_ACTIONS::editActivate )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            m_cursor = controls->GetCursorPosition();

            if( m_dragging )
            {
                wxPoint movement = wxPoint( m_cursor.x, m_cursor.y ) -
                                   selection.Item<BOARD_ITEM>( 0 )->GetPosition();

                totalMovement += movement;

                // Drag items to the current cursor position
                for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
                    selection.Item<BOARD_ITEM>( i )->Move( movement + m_offset );

                updateRatsnest( true );
            }
            else    // Prepare to start dragging
            {
                if( m_selectionTool->CheckLock() || selection.Empty() )
                    break;

                // Save items, so changes can be undone
                if( !isUndoInhibited() )
                {
                    editFrame->OnModify();
                    editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );
                }

                if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    m_cursor = VECTOR2I( selection.Item<BOARD_ITEM>( 0 )->GetPosition() );
                    m_offset.x = 0;
                    m_offset.y = 0;
                }
                else
                {
                    VECTOR2D origin;

                    if( evt->IsDrag( BUT_LEFT ) )
                        origin = getView()->GetGAL()->GetGridPoint( evt->DragOrigin() );
                    else
                        origin = getViewControls()->GetCursorPosition();

                    // Update dragging offset (distance between cursor and the first dragged item)
                    m_offset = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) )->GetPosition() -
                                                         wxPoint( origin.x, origin.y );
                }

                controls->SetAutoPan( true );
                m_dragging = true;
                incUndoInhibit();
            }

            selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
            break; // Finish
    }

    if( m_dragging )
        decUndoInhibit();

    m_dragging = false;
    m_offset.x = 0;
    m_offset.y = 0;

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
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();
    ratsnest->ClearSimple();
    ratsnest->Recalculate();

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
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
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
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

            m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
        }

        item->SetFlags( flags );
    }

    setTransitions();

    return 0;
}


int EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !makeSelection( selection ) || m_selectionTool->CheckLock() )
    {
        setTransitions();

        return 0;
    }

    wxPoint rotatePoint = getModificationPoint( selection );

    // If it is being dragged, then it is already saved with UR_CHANGED flag
    if( !isUndoInhibited() )
    {
        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_ROTATED, rotatePoint );
    }

    for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

        item->Rotate( rotatePoint, editFrame->GetRotationAngle() );

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
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
    setTransitions();

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !makeSelection( selection ) || m_selectionTool->CheckLock() )
    {
        setTransitions();

        return 0;
    }

    wxPoint flipPoint = getModificationPoint( selection );

    if( !isUndoInhibited() )   // If it is being dragged, then it is already saved with UR_CHANGED flag
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
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
    setTransitions();

    return 0;
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( !makeSelection( selection ) || m_selectionTool->CheckLock() )
    {
        setTransitions();

        return 0;
    }

    // Get a copy of the selected items set
    PICKED_ITEMS_LIST selectedItems = selection.items;
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

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

    // Default removal procedure
    case PCB_MODULE_TEXT_T:
    {
        if( m_editModules )
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( aItem );

            switch( text->GetType() )
            {
            case TEXTE_MODULE::TEXT_is_REFERENCE:
                DisplayError( getEditFrame<PCB_BASE_FRAME>(), _( "Cannot delete REFERENCE!" ) );
                return;

            case TEXTE_MODULE::TEXT_is_VALUE:
                DisplayError( getEditFrame<PCB_BASE_FRAME>(), _( "Cannot delete VALUE!" ) );
                return;

            case TEXTE_MODULE::TEXT_is_DIVERS:    // suppress warnings
                break;
            }
        }
    }
    /* no break */

    case PCB_PAD_T:
    case PCB_MODULE_EDGE_T:
        if( m_editModules )
        {
            MODULE* module = static_cast<MODULE*>( aItem->GetParent() );
            module->SetLastEditTime();

            board->m_Status_Pcb = 0; // it is done in the legacy view
            aItem->DeleteStructure();
        }

        return;
        break;

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


int EDIT_TOOL::MoveExact( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !makeSelection( selection ) || m_selectionTool->CheckLock() )
    {
        setTransitions();

        return 0;
    }

    wxPoint translation;
    double rotation = 0;

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    DIALOG_MOVE_EXACT dialog( editFrame, translation, rotation );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        if( !isUndoInhibited() )
        {
            editFrame->OnModify();
            // Record an action of move and rotate
            editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );
        }

        VECTOR2I rp = selection.GetCenter();
        wxPoint rotPoint( rp.x, rp.y );

        for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

            item->Move( translation );
            item->Rotate( rotPoint, rotation );

            if( !m_dragging )
                item->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        updateRatsnest( m_dragging );

        if( m_dragging )
            selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        else
            getModel<BOARD>()->GetRatsnest()->Recalculate();

        if( unselect )
            m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

        m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
    }

    setTransitions();

    return 0;
}


int EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    bool increment = aEvent.IsAction( &COMMON_ACTIONS::duplicateIncrement );

    // first, check if we have a selection, or try to get one
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    // Be sure that there is at least one item that we can modify
    if( !makeSelection( selection ) || selTool->CheckLock() )
    {
        setTransitions();
        return 0;
    }

    // we have a selection to work on now, so start the tool process

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    editFrame->OnModify();

    // prevent other tools making undo points while the duplicate is going on
    // so that if you cancel, you don't get a duplicate object hiding over
    // the original
    incUndoInhibit();

    std::vector<BOARD_ITEM*> old_items;

    for( int i = 0; i < selection.Size(); ++i )
    {
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

        if( item )
            old_items.push_back( item );
    }

    for( unsigned i = 0; i < old_items.size(); ++i )
    {
        BOARD_ITEM* item = old_items[i];

        // Unselect the item, so we won't pick it up again
        // Do this first, so a single-item duplicate will correctly call
        // SetCurItem and show the item properties
        m_toolMgr->RunAction( COMMON_ACTIONS::unselectItem, true, item );

        BOARD_ITEM* new_item = NULL;

        if( m_editModules )
            new_item = editFrame->GetBoard()->m_Modules->DuplicateAndAddItem( item, increment );
        else
            new_item = editFrame->GetBoard()->DuplicateAndAddItem( item, increment );

        if( new_item )
        {
            if( new_item->Type() == PCB_MODULE_T )
            {
                static_cast<MODULE*>( new_item )->RunOnChildren( boost::bind( &KIGFX::VIEW::Add,
                        getView(), _1 ) );
            }

            editFrame->GetGalCanvas()->GetView()->Add( new_item );

            // Select the new item, so we can pick it up
            m_toolMgr->RunAction( COMMON_ACTIONS::selectItem, true, new_item );
        }
    }

    // record the new items as added
    editFrame->SaveCopyInUndoList( selection.items, UR_NEW );

    editFrame->DisplayToolMsg( wxString::Format( _( "Duplicated %d item(s)" ),
            (int) old_items.size() ) );

    // pick up the selected item(s) and start moving
    // this works well for "dropping" copies around
    TOOL_EVENT evt = COMMON_ACTIONS::editActivate.MakeEvent();
    Main( evt );

    // and re-enable undos
    decUndoInhibit();

    setTransitions();

    return 0;
}


int EDIT_TOOL::CreateArray( const TOOL_EVENT& aEvent )
{
    // first, check if we have a selection, or try to get one
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    // Be sure that there is at least one item that we can modify
    if( !makeSelection( selection ) || selTool->CheckLock() )
    {
        setTransitions();
        return 0;
    }

    bool originalItemsModified = false;

    // we have a selection to work on now, so start the tool process

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    editFrame->OnModify();

    if( m_editModules )
    {
        // Module editors do their undo point upfront for the whole module
        editFrame->SaveCopyInUndoList( editFrame->GetBoard()->m_Modules, UR_MODEDIT );
    }
    else
    {
        // We may also change the original item
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );
    }

    DIALOG_CREATE_ARRAY::ARRAY_OPTIONS* array_opts = NULL;

    VECTOR2I rp = selection.GetCenter();
    const wxPoint rotPoint( rp.x, rp.y );

    DIALOG_CREATE_ARRAY dialog( editFrame, rotPoint, &array_opts );
    int ret = dialog.ShowModal();

    if( ret == DIALOG_CREATE_ARRAY::CREATE_ARRAY_OK && array_opts != NULL )
    {
        PICKED_ITEMS_LIST newItemList;

        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );

            if( !item )
                continue;

            wxString cachedString;

            if( item->Type() == PCB_MODULE_T )
            {
                cachedString = static_cast<MODULE*>( item )->GetReferencePrefix();
            }
            else if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
            {
                // Copy the text (not just take a reference
                cachedString = text->GetText();
            }

            // iterate across the array, laying out the item at the
            // correct position
            const unsigned nPoints = array_opts->GetArraySize();

            for( unsigned ptN = 0; ptN < nPoints; ++ptN )
            {
                BOARD_ITEM* newItem = NULL;

                if( ptN == 0 )
                {
                    newItem = item;
                }
                else
                {
                    // if renumbering, no need to increment
                    const bool increment = !array_opts->ShouldRenumberItems();

                    if( m_editModules )
                        newItem = editFrame->GetBoard()->m_Modules->DuplicateAndAddItem( item, increment );
                    else
                        newItem = editFrame->GetBoard()->DuplicateAndAddItem( item, increment );

                    if( newItem )
                    {
                        array_opts->TransformItem( ptN, newItem, rotPoint );

                        m_toolMgr->RunAction( COMMON_ACTIONS::unselectItem, true, newItem );

                        newItemList.PushItem( newItem );

                        if( newItem->Type() == PCB_MODULE_T)
                        {
                            static_cast<MODULE*>( newItem )->RunOnChildren( boost::bind( &KIGFX::VIEW::Add,
                                    getView(), _1 ) );
                        }

                        editFrame->GetGalCanvas()->GetView()->Add( newItem );
                        getModel<BOARD>()->GetRatsnest()->Update( newItem );
                    }
                }

                // set the number if needed:
                if( array_opts->ShouldRenumberItems() )
                {
                    switch( newItem->Type() )
                    {
                    case PCB_PAD_T:
                    {
                        const wxString padName = array_opts->GetItemNumber( ptN );
                        static_cast<D_PAD*>( newItem )->SetPadName( padName );

                        originalItemsModified = true;
                        break;
                    }
                    case PCB_MODULE_T:
                    {
                        const wxString moduleName = array_opts->GetItemNumber( ptN );
                        MODULE* module = static_cast<MODULE*>( newItem );
                        module->SetReference( cachedString + moduleName );

                        originalItemsModified = true;
                        break;
                    }
                    case PCB_MODULE_TEXT_T:
                    case PCB_TEXT_T:
                    {
                        EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( newItem );
                        text->SetText( array_opts->InterpolateNumberIntoString( ptN, cachedString ) );

                        originalItemsModified = true;
                        break;
                    }
                    default:
                        // no renumbering of other items
                        break;
                    }
                }
            }
        }

        if( !m_editModules )
        {
            if( originalItemsModified )
            {
                // Update the appearance of the original items
                selection.group->ItemsViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }

            // Add all items as a single undo point for PCB editors
            // TODO: Can this be merged into the previous undo point (where
            //       we saved the original items)
            editFrame->SaveCopyInUndoList( newItemList, UR_NEW );
        }
    }

    getModel<BOARD>()->GetRatsnest()->Recalculate();
    setTransitions();

    return 0;
}


void EDIT_TOOL::setTransitions()
{
    Go( &EDIT_TOOL::Main,       COMMON_ACTIONS::editActivate.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,     COMMON_ACTIONS::rotate.MakeEvent() );
    Go( &EDIT_TOOL::Flip,       COMMON_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,     COMMON_ACTIONS::remove.MakeEvent() );
    Go( &EDIT_TOOL::Properties, COMMON_ACTIONS::properties.MakeEvent() );
    Go( &EDIT_TOOL::MoveExact,  COMMON_ACTIONS::moveExact.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  COMMON_ACTIONS::duplicate.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  COMMON_ACTIONS::duplicateIncrement.MakeEvent() );
    Go( &EDIT_TOOL::CreateArray,COMMON_ACTIONS::createArray.MakeEvent() );
}


void EDIT_TOOL::updateRatsnest( bool aRedraw )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
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


wxPoint EDIT_TOOL::getModificationPoint( const SELECTION& aSelection )
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


bool EDIT_TOOL::makeSelection( const SELECTION& aSelection )
{
    if( aSelection.Empty() )                        // Try to find an item that could be modified
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionCursor, true );

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
