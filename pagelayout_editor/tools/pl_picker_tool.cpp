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

#include <tool/tool_manager.h>
#include <tool/selection_conditions.h>
#include <tools/pl_picker_tool.h>
#include <tools/pl_actions.h>
#include <view/view_controls.h>
#include <pl_editor_frame.h>

TOOL_ACTION PL_ACTIONS::pickerTool( "plEditor.InteractivePicker", 
        AS_GLOBAL, 0, "", "", "", NULL, AF_ACTIVATE );


PL_PICKER_TOOL::PL_PICKER_TOOL() :
        TOOL_INTERACTIVE( "plEditor.InteractivePicker" ),
        m_frame( nullptr ),
        m_cursorCapture( false ),
        m_autoPanning( false )
{
    resetPicker();
}


bool PL_PICKER_TOOL::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways, 1 );

    // Finally, add the standard zoom/grid items
    m_menu.AddStandardSubMenus( m_frame );

    return true;
}


void PL_PICKER_TOOL::Reset( RESET_REASON aReason )
{
    m_cursorCapture = false;
    m_autoPanning = false;
}


int PL_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    int finalize_state = WAIT_CANCEL;

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

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
                    std::cerr << "PL_PICKER_TOOL click handler error: " << e.what() << std::endl;
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

        else if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& e )
                {
                    std::cerr << "PL_PICKER_TOOL cancel handler error: " << e.what() << std::endl;
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
            // TODO...
            // m_menu.ShowContextMenu();
        }
        else
        {
            m_toolMgr->PassEvent();
        }
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& e )
        {
            std::cerr << "PL_PICKER_TOOL finalize handler error: " << e.what() << std::endl;
        }
    }

    resetPicker();
    controls->ForceCursorPosition( false );
    getEditFrame<PL_EDITOR_FRAME>()->SetNoToolSelected();

    return 0;
}


void PL_PICKER_TOOL::setTransitions()
{
    Go( &PL_PICKER_TOOL::Main, PL_ACTIONS::pickerTool.MakeEvent() );
}


void PL_PICKER_TOOL::resetPicker()
{
    m_cursorCapture = false;
    m_autoPanning = false;

    m_picked = NULLOPT;
    m_clickHandler = NULLOPT;
    m_cancelHandler = NULLOPT;
    m_finalizeHandler = NULLOPT;
}


void PL_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}
