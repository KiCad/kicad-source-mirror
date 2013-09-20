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
#include <tool/tool_manager.h>
#include <tool/tool_action.h>
#include <view/view_controls.h>

#include "selection_tool.h"
#include "move_tool.h"

using namespace KiGfx;
using boost::optional;

MOVE_TOOL::MOVE_TOOL() :
        TOOL_INTERACTIVE( "pcbnew.InteractiveMove" ), m_selectionTool( NULL ),
        m_activate( m_toolName, AS_GLOBAL, 'M', "Move", "Moves the selected item(s)" )
{
}


MOVE_TOOL::~MOVE_TOOL()
{
}


void MOVE_TOOL::Reset()
{
    m_toolMgr->RegisterAction( &m_activate );

    // Find the selection tool, so they can cooperate
    TOOL_BASE* selectionTool = m_toolMgr->FindTool( std::string( "pcbnew.InteractiveSelection" ) );

    if( selectionTool )
    {
        m_selectionTool = static_cast<SELECTION_TOOL*>( selectionTool );
    }
    else
    {
        wxLogError( "pcbnew.InteractiveSelection tool is not available" );
        return;
    }

    // the tool launches upon reception of action event ("pcbnew.InteractiveMove")
    Go( &MOVE_TOOL::Main, m_activate.GetEvent() );
}


int MOVE_TOOL::Main( TOOL_EVENT& aEvent )
{
    VECTOR2D dragPosition;
    bool dragging = false;
    bool restore = false;       // Should items' state be restored when finishing the tool?
    VIEW* view = getView();
    VIEW_CONTROLS* controls = getViewControls();

    view->Add( &m_items );
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            restore = true;
            break;  // Finish
        }

        if( evt->IsMotion() || evt->IsDrag( MB_Left ) )
        {
            if( dragging )
            {
                // Dragging is already active
                VECTOR2D movement = ( evt->Position() - dragPosition );
                std::set<BOARD_ITEM*>::iterator it, it_end;

                // so move all the selected items
                for( it = m_selection.begin(), it_end = m_selection.end(); it != it_end; ++it )
                    (*it)->Move( wxPoint( movement.x, movement.y ) );
            }
            else
            {
                // Prepare to drag
                m_selection = m_selectionTool->GetSelection();
                if( m_selection.empty() )
                    break;  // there are no items to operate on

                std::set<BOARD_ITEM*>::iterator it;
                for( it = m_selection.begin(); it != m_selection.end(); ++it )
                {
                    // Gather all selected items into one VIEW_GROUP
                    viewGroupAdd( *it, &m_items );

                    // Modules are treated in a special way - when they are moved, we have to
                    // move all the parts that make the module, not the module itself
                    if( (*it)->Type() == PCB_MODULE_T )
                    {
                        MODULE* module = static_cast<MODULE*>( *it );

                        // Add everything that belongs to the module (besides the module itself)
                        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
                            viewGroupAdd( pad, &m_items );

                        for( BOARD_ITEM* drawing = module->GraphicalItems().GetFirst(); drawing;
                             drawing = drawing->Next() )
                            viewGroupAdd( drawing, &m_items );

                        viewGroupAdd( &module->Reference(), &m_items );
                        viewGroupAdd( &module->Value(), &m_items );
                    }
                }

                dragging = true;
            }

            m_items.ViewUpdate( VIEW_ITEM::GEOMETRY );
            dragPosition = evt->Position();
        }
        else if( evt->IsMouseUp( MB_Left ) || evt->IsClick( MB_Left ) )
            break;  // Finish
    }

    // Clean-up after movement
    std::deque<ITEM_STATE>::iterator it, it_end;
    if( restore )
    {
        // Movement has to be rollbacked, so restore the previous state of items
        for( it = m_itemsState.begin(), it_end = m_itemsState.end(); it != it_end; ++it )
            it->Restore();
    }
    else
    {
        // Apply changes
        for( it = m_itemsState.begin(), it_end = m_itemsState.end(); it != it_end; ++it )
        {
            it->RestoreVisibility();
            it->item->ViewUpdate( VIEW_ITEM::GEOMETRY );
        }
    }

    m_itemsState.clear();
    m_items.Clear();
    view->Remove( &m_items );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    return 0;
}


void MOVE_TOOL::viewGroupAdd( BOARD_ITEM* aItem, KiGfx::VIEW_GROUP* aGroup )
{
    // Save the state of the selected items, in case it has to be restored
    ITEM_STATE state;
    state.Save( aItem );
    m_itemsState.push_back( state );

    // Add items to the VIEW_GROUP, so they will be displayed on the overlay
    // while dragging
    aGroup->Add( aItem );

    // Set the original item as invisible
    aItem->ViewSetVisible( false );
}
