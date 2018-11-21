/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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

#include "picker_tool.h"
#include "pcb_actions.h"
#include "grid_helper.h"

#include <pcb_edit_frame.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include "tool_event_utils.h"
#include "selection_tool.h"


TOOL_ACTION PCB_ACTIONS::pickerTool( "pcbnew.Picker", AS_GLOBAL, 0, "", "", NULL, AF_ACTIVATE );


PICKER_TOOL::PICKER_TOOL()
    : PCB_TOOL( "pcbnew.Picker" )
{
    reset();
}


bool PICKER_TOOL::Init()
{
    auto activeToolCondition = [ this ] ( const SELECTION& aSel ) {
        return ( frame()->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    // We delegate our context menu to the Selection tool, so make sure it has a
    // "Cancel" item at the top.
    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        menu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1000 );
        menu.AddSeparator( activeToolCondition, 1000 );
    }

    return true;
}


int PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    GRID_HELPER grid( frame() );
    int finalize_state = WAIT_CANCEL;

    assert( !m_picking );
    m_picking = true;
    m_picked = NULLOPT;

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        auto mousePos = controls->GetMousePosition();
        auto p = grid.BestSnapAnchor( mousePos, nullptr );
        controls->ForceCursorPosition( true, p );

        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = VECTOR2D( p );

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& e )
                {
                    std::cerr << "PICKER_TOOL click handler error: " << e.what() << std::endl;
                    finalize_state = EXCEPTION_CANCEL;
                    break;
                }
            }

            if( !getNext )
            {
                finalize_state = CLICK_CANCEL;
                break;
            }
            else
                setControls();
        }
        else if( evt->IsCancel() || TOOL_EVT_UTILS::IsCancelInteractive( *evt ) || evt->IsActivate() )
        {
            finalize_state = EVT_CANCEL;
            break;
        }
        else
            m_toolMgr->PassEvent();
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& e )
        {
            std::cerr << "PICKER_TOOL finalize handler error: " << e.what() << std::endl;
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    getEditFrame<PCB_BASE_FRAME>()->SetNoToolSelected();

    return 0;
}


void PICKER_TOOL::setTransitions()
{
    Go( &PICKER_TOOL::Main, PCB_ACTIONS::pickerTool.MakeEvent() );
}


void PICKER_TOOL::reset()
{
    m_cursorSnapping = true;
    m_cursorVisible = true;
    m_cursorCapture = false;
    m_autoPanning = false;

    m_picking = false;
    m_clickHandler = NULLOPT;
    m_finalizeHandler = NULLOPT;
}


void PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->ShowCursor( m_cursorVisible );
    controls->SetSnapping( m_cursorSnapping );
    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}
