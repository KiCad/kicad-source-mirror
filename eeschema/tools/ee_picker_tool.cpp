/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <ee_picker_tool.h>
#include <ee_actions.h>
#include <view/view_controls.h>
#include <sch_base_frame.h>


EE_PICKER_TOOL::EE_PICKER_TOOL()
    : EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InteractivePicker" )
{
    resetPicker();
}


int EE_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    int                   finalize_state = WAIT_CANCEL;

    // To many things are off-grid in LibEdit; turn snapping off.
    bool                  snap = !m_isLibEdit;

    setControls();

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_BULLSEYE );
        VECTOR2D cursorPos = controls->GetCursorPosition( snap && !evt->Modifier( MD_ALT ) );

        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( cursorPos );
                }
                catch( std::exception& e )
                {
                    std::cerr << "EE_PICKER_TOOL click handler error: " << e.what() << std::endl;
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

        else if( evt->IsMotion() )
        {
            if( m_motionHandler )
            {
                try
                {
                    (*m_motionHandler)( cursorPos );
                }
                catch( std::exception& e )
                {
                    std::cerr << "EE_PICKER_TOOL motion handler error: " << e.what() << std::endl;
                }
            }
        }

        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& e )
                {
                    std::cerr << "EE_PICKER_TOOL cancel handler error: " << e.what() << std::endl;
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
            m_menu.ShowContextMenu();
        }
        else
            evt->SetPassEvent();
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& e )
        {
            std::cerr << "EE_PICKER_TOOL finalize handler error: " << e.what() << std::endl;
        }
    }

    resetPicker();
    controls->ForceCursorPosition( false );
    m_frame->PopTool();
    return 0;
}


void EE_PICKER_TOOL::setTransitions()
{
    Go( &EE_PICKER_TOOL::Main, EE_ACTIONS::pickerTool.MakeEvent() );
}


void EE_PICKER_TOOL::resetPicker()
{
    m_clickHandler = NULLOPT;
    m_motionHandler = NULLOPT;
    m_cancelHandler = NULLOPT;
    m_finalizeHandler = NULLOPT;
}


void EE_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( false );
    controls->SetAutoPan( false );
}
