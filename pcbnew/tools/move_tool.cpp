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
#include <view/view_controls.h>

#include "common_actions.h"
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
    // The tool launches upon reception of action event ("pcbnew.InteractiveMove")
    Go( &MOVE_TOOL::Main, COMMON_ACTIONS::moveActivate.MakeEvent() );
}


bool MOVE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    TOOL_BASE* selectionTool = m_toolMgr->FindTool( "pcbnew.InteractiveSelection" );

    if( selectionTool )
    {
        m_selectionTool = static_cast<SELECTION_TOOL*>( selectionTool );

        // Add context menu entries that are displayed when selection tool is active
        m_selectionTool->AddMenuItem( COMMON_ACTIONS::moveActivate );
        m_selectionTool->AddMenuItem( COMMON_ACTIONS::rotate );
        m_selectionTool->AddMenuItem( COMMON_ACTIONS::flip );
    }
    else
    {
        wxLogError( wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    return true;
}


int MOVE_TOOL::Main( TOOL_EVENT& aEvent )
{
    VECTOR2D dragPosition;
    bool dragging = false;
    bool restore = false;       // Should items' state be restored when finishing the tool?
    VIEW* view = getView();
    VIEW_CONTROLS* controls = getViewControls();

    // Add a VIEW_GROUP that will hold all modified items
    view->Add( &m_items );

    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            restore = true;     // Cancelling the tool means that items have to be restored
            break;  // Finish
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_Command )
        {
            VECTOR2D cursorPos = getView()->ToWorld( getViewControls()->GetCursorPosition() );

            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )           // got rotation event?
            {
                m_state.Rotate( cursorPos, 900.0 );
                m_items.ViewUpdate( VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )        // got flip event?
            {
                m_state.Flip( cursorPos );
                m_items.ViewUpdate( VIEW_ITEM::GEOMETRY );
            }
        }

        else if( evt->IsMotion() || evt->IsDrag( MB_Left ) )
        {
            if( dragging )
            {
                // Drag items to the current cursor position
                VECTOR2D movement = ( evt->Position() - dragPosition );
                m_state.Move( movement );
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
                    // Save the state of the selected items, in case it has to be restored
                    m_state.Save( *it );

                    // Gather all selected items into one VIEW_GROUP
                    viewGroupAdd( *it, &m_items );
                }

                // Hide the original items, they are temporarily shown in VIEW_GROUP on overlay
                vgSetVisibility( &m_items, false );
                vgUpdate( &m_items, VIEW_ITEM::APPEARANCE );

                dragging = true;
            }

            m_items.ViewUpdate( VIEW_ITEM::GEOMETRY );
            dragPosition = evt->Position();
        }
        else if( evt->IsMouseUp( MB_Left ) || evt->IsClick( MB_Left ) )
            break;  // Finish
    }

    // Restore visibility of the original items
    vgSetVisibility( &m_items, true );

    if( restore )
    {
        // Modifications has to be rollbacked, so restore the previous state of items
        vgUpdate( &m_items, VIEW_ITEM::APPEARANCE );
        m_state.RestoreAll();
    }
    else
    {
        // Changes are applied, so update the items
        vgUpdate( &m_items, m_state.GetUpdateFlag() );
        m_state.Apply();
    }

    m_items.Clear();
    view->Remove( &m_items );

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    return 0;
}


void MOVE_TOOL::viewGroupAdd( BOARD_ITEM* aItem, VIEW_GROUP* aGroup )
{
    // Modules are treated in a special way - when they are moved, we have to
    // move all the parts that make the module, not the module itself
    if( aItem->Type() == PCB_MODULE_T )
    {
        MODULE* module = static_cast<MODULE*>( aItem );

        // Add everything that belongs to the module (besides the module itself)
        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
            viewGroupAdd( pad, &m_items );

        for( BOARD_ITEM* drawing = module->GraphicalItems().GetFirst(); drawing;
             drawing = drawing->Next() )
            viewGroupAdd( drawing, &m_items );

        viewGroupAdd( &module->Reference(), &m_items );
        viewGroupAdd( &module->Value(), &m_items );
    }

    // Add items to the VIEW_GROUP, so they will be displayed on the overlay
    // while dragging
    aGroup->Add( aItem );
}


void MOVE_TOOL::vgSetVisibility( VIEW_GROUP* aGroup, bool aVisible ) const
{
    std::set<VIEW_ITEM*>::const_iterator it, it_end;
    for( it = aGroup->Begin(), it_end = aGroup->End(); it != it_end; ++it )
        (*it)->ViewSetVisible( aVisible );
}


void MOVE_TOOL::vgUpdate( VIEW_GROUP* aGroup, VIEW_ITEM::ViewUpdateFlags aFlags ) const
{
    std::set<VIEW_ITEM*>::const_iterator it, it_end;
    for( it = aGroup->Begin(), it_end = aGroup->End(); it != it_end; ++it )
        (*it)->ViewUpdate( aFlags );
}
