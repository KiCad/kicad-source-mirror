/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <cassert>

#include <class_drawpanel_gal.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_track.h>
#include <class_module.h>

#include <wxPcbStruct.h>
#include <collectors.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <painter.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>

#include "selection_tool.h"
#include "selection_area.h"
#include "bright_box.h"
#include "common_actions.h"

using boost::optional;

SELECTION_TOOL::SELECTION_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveSelection" ), m_additive( false ), m_multiple( false )
{
    m_selArea = new SELECTION_AREA;
    m_selection.group = new KIGFX::VIEW_GROUP;
}


SELECTION_TOOL::~SELECTION_TOOL()
{
    delete m_selArea;
    delete m_selection.group;
}


void SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == TOOL_BASE::MODEL_RELOAD )
        // Remove pointers to the selected items from containers
        // without changing their properties (as they are already deleted)
        m_selection.clear();
    else
        // Restore previous properties of selected items and remove them from containers
        clearSelection();

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( m_selection.group );
    getView()->Add( m_selection.group );

    // The tool launches upon reception of action event ("pcbnew.InteractiveSelection")
    Go( &SELECTION_TOOL::Main, COMMON_ACTIONS::selectionActivate.MakeEvent() );
}


int SELECTION_TOOL::Main( TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Should selected items be added to the current selection or
        // become the new selection (discarding previously selected items)
        m_additive = evt->Modifier( MD_SHIFT );

        if( evt->IsAction( &COMMON_ACTIONS::selectionSingle ) )
        {
            selectSingle( getView()->ToWorld( getViewControls()->GetMousePosition() ) );
        }

        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO ||
                 evt->IsAction( &COMMON_ACTIONS::selectionClear ) )
        {
            clearSelection();
        }

        // single click? Select single object
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !m_additive )
                clearSelection();

            selectSingle( evt->Position() );
        }

        // double click? Display the properties window
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            if( m_selection.Empty() )
                selectSingle( evt->Position() );

            m_toolMgr->RunAction( "pcbnew.InteractiveEdit.properties" );
        }

        // drag with LMB? Select multiple objects (or at least draw a selection box) or drag them
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            if( m_selection.Empty() || m_additive )
            {
                // If nothings has been selected or user wants to select more
                // draw the selection box
                selectMultiple();
            }
            else
            {
                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    m_toolMgr->InvokeTool( "pcbnew.InteractiveEdit" );
                }
                else
                {
                    // No -> clear the selection list
                    clearSelection();
                }
            }
        }
    }

    // This tool is supposed to be active forever
    assert( false );

    return 0;
}


void SELECTION_TOOL::AddMenuItem( const TOOL_ACTION& aAction )
{
    assert( aAction.GetId() > 0 );    // Check if the action was registered before in ACTION_MANAGER

    m_menu.Add( aAction );
}


void SELECTION_TOOL::toggleSelection( BOARD_ITEM* aItem )
{
    if( aItem->IsSelected() )
    {
        deselect( aItem );
    }
    else
    {
        if( !m_additive )
            clearSelection();

        // Prevent selection of invisible or inactive items
        if( selectable( aItem ) )
            select( aItem );
    }
}


void SELECTION_TOOL::selectSingle( const VECTOR2I& aWhere )
{
    BOARD* pcb = getModel<BOARD>( PCB_T );
    BOARD_ITEM* item;
    GENERAL_COLLECTORS_GUIDE guide = getEditFrame<PCB_EDIT_FRAME>()->GetCollectorsGuide();
    GENERAL_COLLECTOR collector;

    collector.Collect( pcb, GENERAL_COLLECTOR::AllBoardItems,
                       wxPoint( aWhere.x, aWhere.y ), guide );

    switch( collector.GetCount() )
    {
    case 0:
        if( !m_additive )
            clearSelection();
        break;

    case 1:
        toggleSelection( collector[0] );
        break;

    default:
        // Remove unselectable items
        for( int i = collector.GetCount() - 1; i >= 0 ; --i )
        {
            if( !selectable( collector[i] ) )
                collector.Remove( i );
        }

        // Let's see if there is still disambiguation in selection..
        if( collector.GetCount() == 1 )
        {
            toggleSelection( collector[0] );
        }
        else if( collector.GetCount() > 1 )
        {
            item = disambiguationMenu( &collector );

            if( item )
                toggleSelection( item );
        }

        break;
    }
}


bool SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool cancelled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();
    getViewControls()->SetAutoPan( true );

    view->Add( m_selArea );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_additive )
                clearSelection();

            // Start drawing a selection box
            m_selArea->SetOrigin( evt->DragOrigin() );
            m_selArea->SetEnd( evt->Position() );
            m_selArea->ViewSetVisible( true );
            m_selArea->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            // End drawing the selection box
            m_selArea->ViewSetVisible( false );

            // Mark items within the selection box as selected
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;
            BOX2I selectionBox = m_selArea->ViewBBox();
            view->Query( selectionBox, selectedItems );         // Get the list of selected items

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR>::iterator it, it_end;

            for( it = selectedItems.begin(), it_end = selectedItems.end(); it != it_end; ++it )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it->first );

                // Add only those items that are visible and fully within the selection box
                if( !item->IsSelected() && selectable( item ) && selectionBox.Contains( item->ViewBBox() ) )
                    select( item );
            }

            // Do not display information about selected item,as there is more than one
            getEditFrame<PCB_EDIT_FRAME>()->SetCurItem( NULL );

            break;  // Stop waiting for events
        }
    }

    view->Remove( m_selArea );
    m_multiple = false;         // Multiple selection mode is inactive
    getViewControls()->SetAutoPan( false );

    return cancelled;
}


void SELECTION_TOOL::clearSelection()
{
    if( m_selection.Empty() )
        return;

    KIGFX::VIEW_GROUP::const_iter it, it_end;

    // Restore the initial properties
    for( it = m_selection.group->Begin(), it_end = m_selection.group->End(); it != it_end; ++it )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( *it );

        item->ViewSetVisible( true );
        item->ClearSelected();
    }
    m_selection.clear();

    getEditFrame<PCB_EDIT_FRAME>()->SetCurItem( NULL );

    // Do not show the context menu when there is nothing selected
    SetContextMenu( &m_menu, CMENU_OFF );
}


BOARD_ITEM* SELECTION_TOOL::disambiguationMenu( GENERAL_COLLECTOR* aCollector )
{
    BOARD_ITEM* current = NULL;
    boost::shared_ptr<BRIGHT_BOX> brightBox;
    CONTEXT_MENU menu;

    int limit = std::min( 10, aCollector->GetCount() );

    for( int i = 0; i < limit; ++i )
    {
        wxString text;
        BOARD_ITEM* item = ( *aCollector )[i];
        text = item->GetSelectMenuText();
        menu.Add( text, i );
    }

    menu.SetTitle( _( "Clarify selection" ) );
    SetContextMenu( &menu, CMENU_NOW );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->Action() == TA_CONTEXT_MENU_UPDATE )
        {
            if( current )
                current->ClearBrightened();

            int id = *evt->GetCommandId();

            // User has pointed an item, so show it in a different way
            if( id >= 0 && id < limit )
            {
                current = ( *aCollector )[id];
                current->SetBrightened();
            }
            else
                current = NULL;
        }
        else if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
        {
            optional<int> id = evt->GetCommandId();

            // User has selected an item, so this one will be returned
            if( id && ( *id >= 0 ) )
                current = ( *aCollector )[*id];

            break;
        }

        // Draw a mark to show which item is available to be selected
        if( current && current->IsBrightened() )
        {
            brightBox.reset( new BRIGHT_BOX( current ) );
            getView()->Add( brightBox.get() );
        }
    }

    // Removes possible brighten mark
    getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );

    return current;
}


BOARD_ITEM* SELECTION_TOOL::pickSmallestComponent( GENERAL_COLLECTOR* aCollector )
{
    int count = aCollector->GetPrimaryCount();     // try to use preferred layer

    if( 0 == count )
        count = aCollector->GetCount();

    for( int i = 0; i < count; ++i )
    {
        if( ( *aCollector )[i]->Type() != PCB_MODULE_T )
            return NULL;
    }

    // All are modules, now find smallest MODULE
    int minDim = 0x7FFFFFFF;
    int minNdx = 0;

    for( int i = 0; i < count; ++i )
    {
        MODULE* module = (MODULE*) ( *aCollector )[i];

        int lx = module->GetBoundingBox().GetWidth();
        int ly = module->GetBoundingBox().GetHeight();

        int lmin = std::min( lx, ly );

        if( lmin < minDim )
        {
            minDim = lmin;
            minNdx = i;
        }
    }

    return (*aCollector)[minNdx];
}


bool SELECTION_TOOL::selectable( const BOARD_ITEM* aItem ) const
{
    // Is high contrast mode enabled?
    bool highContrast = getView()->GetPainter()->GetSettings()->GetHighContrast();

    if( highContrast )
    {
        bool onActive = false;          // Is the item on any of active layers?
        int layers[KIGFX::VIEW::VIEW_MAX_LAYERS], layers_count;

        // Filter out items that do not belong to active layers
        std::set<unsigned int> activeLayers = getView()->GetPainter()->
                                              GetSettings()->GetActiveLayers();
        aItem->ViewGetLayers( layers, layers_count );

        for( int i = 0; i < layers_count; ++i )
        {
            if( activeLayers.count( layers[i] ) > 0 )    // Item is on at least one of active layers
            {
                onActive = true;
                break;
            }
        }

        if( !onActive ) // We do not want to select items that are in the background
            return false;
    }

    BOARD* board = getModel<BOARD>( PCB_T );

    switch( aItem->Type() )
    {
    case PCB_VIA_T:
    {
        // For vias it is enough if only one of layers is visible
        LAYER_NUM top, bottom;
        static_cast<const SEGVIA*>( aItem )->ReturnLayerPair( &top, &bottom );

        return board->IsLayerVisible( top ) || board->IsLayerVisible( bottom );
    }
    break;

    case PCB_MODULE_T:
        if( aItem->IsOnLayer( LAYER_N_FRONT ) && board->IsElementVisible( MOD_FR_VISIBLE ) )
            return true;

        if( aItem->IsOnLayer( LAYER_N_BACK ) && board->IsElementVisible( MOD_BK_VISIBLE ) )
            return true;

        return false;

        break;

    case PCB_MODULE_TEXT_T:
        if( m_multiple )
            return false;
        break;

    // These are not selectable
    case PCB_MODULE_EDGE_T:
    case PCB_PAD_T:
    case NOT_USED:
    case TYPE_NOT_INIT:
        return false;

    default:    // Suppress warnings
        break;
    }

    // All other items are selected only if the layer on which they exist is visible
    return board->IsLayerVisible( aItem->GetLayer() );
}


void SELECTION_TOOL::select( BOARD_ITEM* aItem )
{
    // Modules are treated in a special way - when they are selected, we have to mark
    // all the parts that make the module as selected
    if( aItem->Type() == PCB_MODULE_T )
    {
        MODULE* module = static_cast<MODULE*>( aItem );
        module->RunOnChildren( std::bind1st( std::mem_fun( &SELECTION_TOOL::selectVisually ), this ) );
    }

    selectVisually( aItem );
    ITEM_PICKER picker( aItem );
    m_selection.items.PushItem( picker );

    // It is enough to do it only for the first selected item
    if( m_selection.Size() == 1 )
    {
        // Set as the current item, so the information about selection is displayed
        getEditFrame<PCB_EDIT_FRAME>()->SetCurItem( aItem, true );

        // Now the context menu should be enabled
        SetContextMenu( &m_menu, CMENU_BUTTON );
    }
    else if( m_selection.Size() == 2 )  // Check only for 2, so it will not be
    {                                   // called for every next selected item
        // If multiple items are selected, do not show the information about the selected item
        getEditFrame<PCB_EDIT_FRAME>()->SetCurItem( NULL, true );
    }
}


void SELECTION_TOOL::deselect( BOARD_ITEM* aItem )
{
    // Modules are treated in a special way - when they are selected, we have to
    // deselect all the parts that make the module, not the module itself
    if( aItem->Type() == PCB_MODULE_T )
    {
        MODULE* module = static_cast<MODULE*>( aItem );
        module->RunOnChildren( std::bind1st( std::mem_fun( &SELECTION_TOOL::deselectVisually ), this ) );
    }

    deselectVisually( aItem );

    int itemIdx = m_selection.items.FindItem( aItem );
    if( itemIdx >= 0 )
        m_selection.items.RemovePicker( itemIdx );

    // If there is nothing selected, disable the context menu
    if( m_selection.Empty() )
    {
        SetContextMenu( &m_menu, CMENU_OFF );
        getEditFrame<PCB_EDIT_FRAME>()->SetCurItem( NULL );
    }
}


void SELECTION_TOOL::selectVisually( BOARD_ITEM* aItem ) const
{
    m_selection.group->Add( aItem );

    // Hide the original item, so it is shown only on overlay
    aItem->ViewSetVisible( false );
    aItem->SetSelected();
}


void SELECTION_TOOL::deselectVisually( BOARD_ITEM* aItem ) const
{
    m_selection.group->Remove( aItem );

    // Restore original item visibility
    aItem->ViewSetVisible( true );
    aItem->ClearSelected();
}


bool SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    VECTOR2D margin = getView()->ToWorld( VECTOR2D( GRIP_MARGIN, GRIP_MARGIN ), false );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( unsigned int i = 0; i < m_selection.items.GetCount(); ++i )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( m_selection.items.GetPickedItem( i ) );
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin.x, margin.y );    // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


void SELECTION_TOOL::SELECTION::clear()
{
    items.ClearItemsList();
    group->Clear();
}
