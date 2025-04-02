/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <functional>
using namespace std::placeholders;

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_fpviewer_selection_tool.h>


CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "common.InteractiveSelection" ),
        m_frame( nullptr )
{
}


bool CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Init()
{
    getEditFrame<DISPLAY_FOOTPRINTS_FRAME>()->AddStandardSubMenus( *m_menu.get() );
    return true;
}


void CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<DISPLAY_FOOTPRINTS_FRAME>();
}


int CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( m_frame->ToolStackIsEmpty() )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

        if( evt->IsClick( BUT_LEFT ) )
        {
            clearSelection();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selection );
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
        }
        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    return 0;
}


int CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu      = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( m_selection );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


void CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::setTransitions()
{
    Go( &CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::UpdateMenu, ACTIONS::updateMenu.MakeEvent() );
    Go( &CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Main, ACTIONS::selectionActivate.MakeEvent() );
}
