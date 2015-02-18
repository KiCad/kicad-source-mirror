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

#include <limits>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <wxPcbStruct.h>
#include <kiway.h>
#include <module_editor_frame.h>

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
#include "grid_helper.h"

EDIT_TOOL::EDIT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveEdit" ), m_selectionTool( NULL ),
    m_dragging( false ), m_editModules( false ), m_updateFlag( KIGFX::VIEW_ITEM::NONE )
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

    // Footprint actions
    m_selectionTool->AddMenuItem( COMMON_ACTIONS::editFootprintInFpEditor,
                                SELECTION_CONDITIONS::OnlyType ( PCB_MODULE_T ) &&
                                SELECTION_CONDITIONS::Count ( 1 ) );

    m_offset.x = 0;
    m_offset.y = 0;

    setTransitions();

    return true;
}

bool EDIT_TOOL::invokeInlineRouter()
{
    TRACK *track = uniqueSelected<TRACK>();
    VIA *via = uniqueSelected<VIA>();

    if( track || via )
    {
        //printf("Calling interactive drag\n");
        m_toolMgr->RunAction( COMMON_ACTIONS::routerInlineDrag, true );
        return true;
    }

    return false;
}

int EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection)
    if( !hoverSelection( selection ) )
    {
        setTransitions();
        return 0;
    }

    Activate();

    m_dragging = false;         // Are selected items being dragged?
    bool restore = false;       // Should items' state be restored when finishing the tool?
    bool lockOverride = false;
    bool isDragAndDrop = false;

    // By default, modified items need to update their geometry
    m_updateFlag = KIGFX::VIEW_ITEM::GEOMETRY;

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    controls->ShowCursor( true );
    //controls->SetSnapping( true );
    controls->ForceCursorPosition( false );

    GRID_HELPER grid ( editFrame );

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
            //if ( invokeInlineRouter ( ) )
             //   break;

            VECTOR2I mousePos = evt->Position();

            m_cursor = grid.Align( evt->Position() );
            isDragAndDrop = evt->IsDrag( BUT_LEFT );            
            
            if( m_dragging )
            {
                m_cursor = grid.BestSnapAnchor( evt->Position(), selection.Item<BOARD_ITEM>( 0 ) );
                getViewControls()->ForceCursorPosition ( true, m_cursor );

                wxPoint movement = wxPoint( m_cursor.x, m_cursor.y ) -
                                   selection.Item<BOARD_ITEM>( 0 )->GetPosition();

                // Drag items to the current cursor position
                for( unsigned int i = 0; i < selection.items.GetCount(); ++i )
                    selection.Item<BOARD_ITEM>( i )->Move( movement + m_offset );

                updateRatsnest( true );
            }
            else    // Prepare to start dragging
            {
                m_selectionTool->SanitizeSelection();

                if( selection.Empty() )
                    break;

                // deal with locked items (override lock or abort the operation)
                SELECTION_LOCK_FLAGS lockFlags = m_selectionTool->CheckLock();

                if( lockFlags == SELECTION_LOCKED )
                    break;
                else if( lockFlags == SELECTION_LOCK_OVERRIDE )
                    lockOverride = true;

                // Save items, so changes can be undone
                editFrame->OnModify();
                editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

                VECTOR2I origin;

                if( evt->IsDrag( BUT_LEFT ) )
                    mousePos = evt->DragOrigin();

                //    origin = grid.Align ( evt->DragOrigin() );
                //else
                origin = grid.Align( mousePos );

                if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    m_cursor = grid.BestDragOrigin( mousePos, selection.Item<BOARD_ITEM>( 0 ) );
                    getViewControls()->ForceCursorPosition ( true, m_cursor );
                    grid.SetAuxAxes( true, m_cursor );

                    VECTOR2I o = VECTOR2I( selection.Item<BOARD_ITEM>( 0 )->GetPosition() );
                    m_offset.x = o.x - m_cursor.x;
                    m_offset.y = o.y - m_cursor.y;
                }
                else
                {
                    m_offset = static_cast<BOARD_ITEM*>( selection.items.GetPickedItem( 0 ) )->GetPosition() -
                                                         wxPoint( origin.x, origin.y );
                    getViewControls()->ForceCursorPosition( true, origin );
                }

                controls->SetAutoPan( true );
                m_dragging = true;
            }

            selection.group->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            if( !isDragAndDrop || !lockOverride )
                break; // Finish

            lockOverride = false;
        }
    }

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
    //controls->SetSnapping( false );
    controls->SetAutoPan( false );

    setTransitions();

    return 0;
}


int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( !hoverSelection( selection, false ) )
    {
        setTransitions();

        return 0;
    }

    // Properties are displayed when there is only one item selected
    if( selection.Size() == 1 )
    {
        // Display properties dialog
        BOARD_ITEM* item = selection.Item<BOARD_ITEM>( 0 );

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

    if( !hoverSelection( selection ) )
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

    if( !hoverSelection( selection ) )
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
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( COMMON_ACTIONS::pointEditorUpdate, true );
    setTransitions();

    return 0;
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( !hoverSelection( selection ) )
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
        TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( aItem );

        switch( text->GetType() )
        {
            case TEXTE_MODULE::TEXT_is_REFERENCE:
                DisplayError( getEditFrame<PCB_BASE_FRAME>(), _( "Cannot delete component reference." ) );
                return;

            case TEXTE_MODULE::TEXT_is_VALUE:
                DisplayError( getEditFrame<PCB_BASE_FRAME>(), _( "Cannot delete component value." ) );
                return;

            case TEXTE_MODULE::TEXT_is_DIVERS:    // suppress warnings
                break;
        }

        if( m_editModules )
        {
            MODULE* module = static_cast<MODULE*>( aItem->GetParent() );
            module->SetLastEditTime();
            board->m_Status_Pcb = 0; // it is done in the legacy view
            aItem->DeleteStructure();
        }

        return;
    }

    case PCB_PAD_T:
    case PCB_MODULE_EDGE_T:
    {
        MODULE* module = static_cast<MODULE*>( aItem->GetParent() );
        module->SetLastEditTime();

        board->m_Status_Pcb = 0; // it is done in the legacy view


        if( !m_editModules )
        {
            if( aItem->Type() == PCB_PAD_T && module->GetPadCount() == 1 )
            {
                DisplayError( getEditFrame<PCB_BASE_FRAME>(), _( "Cannot delete the only remaining pad of the module (modules on PCB must have at least one pad)." ) );
                return;
            }

            getView()->Remove( aItem );
            board->Remove( aItem );
        }

        aItem->DeleteStructure();

        return;
    }

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

    Go( &EDIT_TOOL::editFootprintInFpEditor, COMMON_ACTIONS::editFootprintInFpEditor.MakeEvent() );
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

bool EDIT_TOOL::hoverSelection( const SELECTION& aSelection, bool aSanitize )
{
    if( aSelection.Empty() )                        // Try to find an item that could be modified
    {
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionCursor, true );

        if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        {
            m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
            return false;
        }
    }

    if( aSanitize )
        m_selectionTool->SanitizeSelection();

    if( aSelection.Empty() )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

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


int EDIT_TOOL::editFootprintInFpEditor( const TOOL_EVENT& aEvent )
{
    MODULE *mod = uniqueSelected<MODULE>();
    
    if( !mod )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    
    editFrame-> SetCurItem( mod );

    if( editFrame->GetCurItem()->GetTimeStamp() == 0 )    // Module Editor needs a non null timestamp
    {
        editFrame->GetCurItem()->SetTimeStamp( GetNewTimeStamp() );
        editFrame->OnModify();
    }

    FOOTPRINT_EDIT_FRAME* editor = (FOOTPRINT_EDIT_FRAME*) editFrame->Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );

    editor->Load_Module_From_BOARD( (MODULE*) editFrame->GetCurItem() );
    editFrame->SetCurItem( NULL );     // the current module could be deleted by

    editor->Show( true );
    editor->Raise();        // Iconize( false );

    setTransitions();
    return 0;
}
