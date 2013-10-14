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

using namespace KIGFX;
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
    const SELECTION_TOOL::SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return 0; // there are no items to operate on

    VECTOR2D dragPosition;
    bool dragging = false;
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
        else if( evt->Category() == TC_Command )
        {
            VECTOR2D cursorPos = getView()->ToWorld( getViewControls()->GetCursorPosition() );

            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )           // got rotation event?
            {
                m_state.Rotate( cursorPos, 900.0 );
                selection.group->ViewUpdate( VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )        // got flip event?
            {
                m_state.Flip( cursorPos );
                selection.group->ViewUpdate( VIEW_ITEM::GEOMETRY );
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
                std::set<BOARD_ITEM*>::iterator it;

                for( it = selection.items.begin(); it != selection.items.end(); ++it )
                {
                    // Save the state of the selected items, in case it has to be restored
                    m_state.Save( *it );
                }

                dragging = true;
            }

            selection.group->ViewUpdate( VIEW_ITEM::GEOMETRY );
            dragPosition = evt->Position();
        }
        else if( evt->IsMouseUp( MB_Left ) || evt->IsClick( MB_Left ) )
            break; // Finish
    }

    if( restore )
    {
        // Modifications has to be rollbacked, so restore the previous state of items
        selection.group->ItemsViewUpdate( VIEW_ITEM::APPEARANCE );
        m_state.RestoreAll();
    }
    else
    {
        // Changes are applied, so update the items
        selection.group->ItemsViewUpdate( m_state.GetUpdateFlag() );
        m_state.Apply();
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    return 0;
}
