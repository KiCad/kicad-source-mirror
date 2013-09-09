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
#include <view/view_group.h>

#include "selection_tool.h"
#include "move_tool.h"

using namespace KiGfx;
using boost::optional;

MOVE_TOOL::MOVE_TOOL() :
        TOOL_INTERACTIVE( "pcbnew.InteractiveMove" ), m_selectionTool( NULL )
{
}


MOVE_TOOL::~MOVE_TOOL()
{
}


void MOVE_TOOL::Reset()
{
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

    // the tool launches upon reception of activate ("pcbnew.InteractiveMove")
    Go( &MOVE_TOOL::Main, TOOL_EVENT( TC_Command, TA_ActivateTool, GetName() ) ); //"pcbnew.InteractiveMove"));
}


int MOVE_TOOL::Main( TOOL_EVENT& aEvent )
{
    VECTOR2D dragPosition;
    bool dragging = false;
    bool restore = false;
    VIEW* view = m_toolMgr->GetView();
    std::set<BOARD_ITEM*> selection;
    VIEW_GROUP items( view );

    view->Add( &items );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            restore = true;
            m_toolMgr->PassEvent();
            break;  // Finish
        }

        if( evt->IsDrag( MB_Left ) )
        {
            if( dragging )
            {
                // Dragging is alre
                VECTOR2D movement = ( evt->Position() - dragPosition );

                std::set<BOARD_ITEM*>::iterator it, it_end;
                for( it = selection.begin(), it_end = selection.end(); it != it_end; ++it )
                {
                    (*it)->Move( wxPoint( movement.x, movement.y ) );
                }
                items.ViewUpdate( VIEW_ITEM::GEOMETRY );
            }
            else
            {
                // Begin dragging
                selection = m_selectionTool->GetSelection();

                std::set<BOARD_ITEM*>::iterator it;
                for( it = selection.begin(); it != selection.end(); ++it )
                {
                    viewGroupAdd( *it, &items );

                    // but if a MODULE was selected, then we need to redraw all of it's parts
                    if( (*it)->Type() == PCB_MODULE_T )
                    {
                        MODULE* module = static_cast<MODULE*>( *it );

                        // Move everything that belongs to the module
                        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
                            viewGroupAdd( pad, &items );

                        for( BOARD_ITEM* drawing = module->GraphicalItems().GetFirst(); drawing;
                             drawing = drawing->Next() )
                            viewGroupAdd( drawing, &items );

                        viewGroupAdd( &module->Reference(), &items );
                        viewGroupAdd( &module->Value(), &items );
                    }

                }
                items.ViewUpdate( VIEW_ITEM::GEOMETRY );

                dragging = true;
            }

            dragPosition = evt->Position();
        }
        else if( evt->Category() == TC_Mouse ) // Filter out other events
        {
            if( dragging )
            {
                break;  // Finish
            }
        }
    }

    // Clean-up after movement
    std::deque<ITEM_STATE>::iterator it, it_end;
    if( restore )
    {
        // Movement has to be rollbacked, so restore previous state of items
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
    items.Clear();
    view->Remove( &items );

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
