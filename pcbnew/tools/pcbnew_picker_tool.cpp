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

#include "pcbnew_picker_tool.h"
#include "pcb_actions.h"
#include "grid_helper.h"
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include "tool_event_utils.h"
#include "selection_tool.h"

TOOL_ACTION PCB_ACTIONS::pickerTool( "pcbnew.InteractivePicker", 
        AS_GLOBAL, 0, "", "", "", nullptr, AF_ACTIVATE );


PCBNEW_PICKER_TOOL::PCBNEW_PICKER_TOOL()
    : PCB_TOOL_BASE( "pcbnew.InteractivePicker" )
{
    reset();
}


int PCBNEW_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    GRID_HELPER grid( frame() );
    int finalize_state = WAIT_CANCEL;

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( controls->GetMousePosition(), nullptr );
        controls->ForceCursorPosition(true, cursorPos );

        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = cursorPos;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& e )
                {
                    std::cerr << "PCBNEW_PICKER_TOOL clickHandler error: " << e.what() << std::endl;
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

        else if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& e )
                {
                    std::cerr << "PCBNEW_PICKER_TOOL cancelHandler error: " << e.what() << std::endl;
                }
            }

            // Activating a new tool may have alternate finalization from canceling the current tool
            if( evt->IsActivate() )
                finalize_state = END_ACTIVATE;
            else
                finalize_state = EVT_CANCEL;

            break;
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            PCBNEW_SELECTION dummy;
            m_menu.ShowContextMenu( dummy );
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
            std::cerr << "PCBNEW_PICKER_TOOL finalizeHandler error: " << e.what() << std::endl;
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    getEditFrame<PCB_BASE_FRAME>()->SetNoToolSelected();

    return 0;
}


void PCBNEW_PICKER_TOOL::setTransitions()
{
    Go( &PCBNEW_PICKER_TOOL::Main, PCB_ACTIONS::pickerTool.MakeEvent() );
}


void PCBNEW_PICKER_TOOL::reset()
{
    m_cursorCapture = false;
    m_autoPanning = false;
    m_layerMask = LSET::AllLayersMask();

    m_picked = NULLOPT;
    m_clickHandler = NULLOPT;
    m_cancelHandler = NULLOPT;
    m_finalizeHandler = NULLOPT;
}


void PCBNEW_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    // Ensure that the view controls do not handle our snapping as we use the GRID_HELPER
    controls->SetSnapping( false );

    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}
