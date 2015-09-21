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
#include "common_actions.h"

#include <wxPcbStruct.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>

PICKER_TOOL::PICKER_TOOL()
    : TOOL_INTERACTIVE( "pcbnew.Picker" )
{
    reset();
}


int PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    assert( !m_picking );
    m_picking = true;
    m_picked = boost::none;

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;
            m_picked = controls->GetCursorPosition();

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& e )
                {
                    std::cerr << "PICKER_TOOL click handler error: " << e.what() << std::endl;
                    break;
                }
            }

            if( !getNext )
                break;
            else
                setControls();
        }

        else if( evt->IsCancel() || evt->IsActivate() )
            break;
    }

    reset();
    getEditFrame<PCB_BASE_FRAME>()->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


void PICKER_TOOL::SetTransitions()
{
    Go( &PICKER_TOOL::Main, COMMON_ACTIONS::pickerTool.MakeEvent() );
}


void PICKER_TOOL::reset()
{
    m_cursorSnapping = true;
    m_cursorVisible = true;
    m_cursorCapture = false;
    m_autoPanning = false;

    m_picking = false;
    m_clickHandler = boost::none;
}


void PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->ShowCursor( m_cursorVisible );
    controls->SetSnapping( m_cursorSnapping );
    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}
