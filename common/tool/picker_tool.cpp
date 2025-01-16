/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/actions.h>
#include <tool/picker_tool.h>
#include <view/view_controls.h>
#include <eda_draw_frame.h>
#include <wx/debug.h>


void PICKER_TOOL_BASE::reset()
{
    m_cursor = KICURSOR::ARROW;
    m_snap   = true;

    m_picked = std::nullopt;
    m_clickHandler = std::nullopt;
    m_motionHandler = std::nullopt;
    m_cancelHandler = std::nullopt;
    m_finalizeHandler = std::nullopt;
}


PICKER_TOOL::PICKER_TOOL( const std::string& aName ) :
        TOOL_INTERACTIVE( aName ),
        PICKER_TOOL_BASE()
{
}


PICKER_TOOL::PICKER_TOOL() :
        TOOL_INTERACTIVE( "common.InteractivePicker" ),
        PICKER_TOOL_BASE()
{
}


bool PICKER_TOOL::Init()
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();

    auto& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    // Finally, add the standard zoom/grid items
    m_frame->AddStandardSubMenus( *m_menu.get() );

    return true;
}


int PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    int finalize_state = WAIT_CANCEL;

    wxCHECK_MSG( aEvent.Parameter<const TOOL_EVENT*>(), -1,
                 wxT( "PICKER_TOOL::Main() called without a source event" ) );

    const TOOL_EVENT sourceEvent = *aEvent.Parameter<const TOOL_EVENT*>();

    m_frame->PushTool( sourceEvent );
    Activate();

    setControls();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( m_cursor );
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        VECTOR2D cursorPos = controls->GetCursorPosition( m_snap && m_frame->IsGridVisible() );
        m_modifiers = aEvent.Modifier();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& )
                {
                }
            }

            // Activating a new tool may have alternate finalization from canceling the current
            // tool
            if( evt->IsActivate() )
            {
                finalize_state = END_ACTIVATE;
            }
            else
            {
                evt->SetPassEvent( false );
                finalize_state = EVT_CANCEL;
            }

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = cursorPos;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& )
                {
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
            {
                setControls();
            }
        }
        else if( evt->IsMotion() )
        {
            if( m_motionHandler )
            {
                try
                {
                    (*m_motionHandler)( cursorPos );
                }
                catch( std::exception& )
                {
                }
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) || evt->IsDrag( BUT_LEFT ) )
        {
            // Not currently used, but we don't want to pass them either
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& )
        {
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    m_frame->PopTool( sourceEvent );
    return 0;
}


void PICKER_TOOL::setTransitions()
{
    Go( &PICKER_TOOL::Main, ACTIONS::pickerTool.MakeEvent() );
}


void PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( false );
    controls->SetAutoPan( false );
}
