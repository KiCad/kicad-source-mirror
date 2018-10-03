/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2007-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cstdint>

#include <view/view.h>
#include "cvpcb_actions.h"
#include "cvpcb_control.h"

#include <class_board.h>

#include <hotkeys.h>
#include <properties.h>

#include <cvpcb_id.h>
#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <tools/grid_helper.h>  // from pcbnew

#include <functional>
using namespace std::placeholders;


// Miscellaneous
TOOL_ACTION CVPCB_ACTIONS::resetCoords( "cvpcb.Control.resetCoords",
        AS_GLOBAL, ' ',//TOOL_ACTION::LegacyHotKey( HK_RESET_LOCAL_COORD ),
        "", "" );

TOOL_ACTION CVPCB_ACTIONS::switchCursor( "cvpcb.Control.switchCursor",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION CVPCB_ACTIONS::switchUnits( "cvpcb.Control.switchUnits",
        AS_GLOBAL, 'U',//TOOL_ACTION::LegacyHotKey( HK_SWITCH_UNITS ),
        "", "" );

TOOL_ACTION CVPCB_ACTIONS::no_selectionTool( "cvpcb.Control.no_selectionTool",
        AS_GLOBAL, ESC,
        "", "", NULL, AF_ACTIVATE );


///////////////
CVPCB_CONTROL::CVPCB_CONTROL() :
    TOOL_INTERACTIVE( "cvpcb.Control" ), m_frame( NULL )
{
}


CVPCB_CONTROL::~CVPCB_CONTROL()
{
}


void CVPCB_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<DISPLAY_FOOTPRINTS_FRAME>();
}


// Miscellaneous
int CVPCB_CONTROL::ResetCoords( const TOOL_EVENT& aEvent )
{
    auto vcSettings = m_toolMgr->GetCurrentToolVC();

    // Use either the active tool forced cursor position or the general settings
    VECTOR2I cursorPos = vcSettings.m_forceCursorPosition ? vcSettings.m_forcedPosition :
                         getViewControls()->GetCursorPosition();

    m_frame->GetScreen()->m_O_Curseur = wxPoint( cursorPos.x, cursorPos.y );
    m_frame->UpdateStatusBar();

    return 0;
}


int CVPCB_CONTROL::SwitchCursor( const TOOL_EVENT& aEvent )
{
    auto& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.m_fullscreenCursor = !galOpts.m_fullscreenCursor;
    galOpts.NotifyChanged();

    return 0;
}


int CVPCB_CONTROL::SwitchUnits( const TOOL_EVENT& aEvent )
{
    // TODO should not it be refactored to pcb_frame member function?
    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );

    if( m_frame->GetUserUnits() == INCHES )
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_MM );
    else
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_INCH );

    m_frame->ProcessEvent( evt );

    return 0;
}


void CVPCB_CONTROL::setTransitions()
{
    // Miscellaneous
    Go( &CVPCB_CONTROL::ResetCoords,        CVPCB_ACTIONS::resetCoords.MakeEvent() );
    Go( &CVPCB_CONTROL::SwitchCursor,       CVPCB_ACTIONS::switchCursor.MakeEvent() );
    Go( &CVPCB_CONTROL::SwitchUnits,        CVPCB_ACTIONS::switchUnits.MakeEvent() );
}
