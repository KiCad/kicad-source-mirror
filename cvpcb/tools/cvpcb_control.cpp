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
//#include <gal/graphics_abstraction_layer.h>
#include <view/view_controls.h>
//#include <pcb_painter.h>
#include <tools/grid_helper.h>  // from pcbnew

#include <functional>
using namespace std::placeholders;

// Cursor control
TOOL_ACTION CVPCB_ACTIONS::cursorUp( "cvpcb.Control.cursorUp",
        AS_GLOBAL, WXK_UP, "", "", NULL, AF_NONE, (void*) CURSOR_UP );
TOOL_ACTION CVPCB_ACTIONS::cursorDown( "cvpcb.Control.cursorDown",
        AS_GLOBAL, WXK_DOWN, "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );
TOOL_ACTION CVPCB_ACTIONS::cursorLeft( "cvpcb.Control.cursorLeft",
        AS_GLOBAL, WXK_LEFT, "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );
TOOL_ACTION CVPCB_ACTIONS::cursorRight( "cvpcb.Control.cursorRight",
        AS_GLOBAL, WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );

TOOL_ACTION CVPCB_ACTIONS::cursorUpFast( "cvpcb.Control.cursorUpFast",
        AS_GLOBAL, MD_CTRL + WXK_UP, "", "", NULL, AF_NONE, (void*) ( CURSOR_UP | CURSOR_FAST_MOVE ) );
TOOL_ACTION CVPCB_ACTIONS::cursorDownFast( "cvpcb.Control.cursorDownFast",
        AS_GLOBAL, MD_CTRL + WXK_DOWN, "", "" , NULL, AF_NONE, (void*) ( CURSOR_DOWN | CURSOR_FAST_MOVE ) );
TOOL_ACTION CVPCB_ACTIONS::cursorLeftFast( "cvpcb.Control.cursorLeftFast",
        AS_GLOBAL, MD_CTRL + WXK_LEFT, "", "" , NULL, AF_NONE, (void*) ( CURSOR_LEFT | CURSOR_FAST_MOVE ) );
TOOL_ACTION CVPCB_ACTIONS::cursorRightFast( "cvpcb.Control.cursorRightFast",
        AS_GLOBAL, MD_CTRL + WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) ( CURSOR_RIGHT | CURSOR_FAST_MOVE ) );

TOOL_ACTION CVPCB_ACTIONS::cursorClick( "cvpcb.Control.cursorClick",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEFT_CLICK ),
        "", "", NULL, AF_NONE, (void*) CURSOR_CLICK );

TOOL_ACTION CVPCB_ACTIONS::panUp( "cvpcb.Control.panUp",
        AS_GLOBAL, MD_SHIFT + WXK_UP, "", "", NULL, AF_NONE, (void*) CURSOR_UP );
TOOL_ACTION CVPCB_ACTIONS::panDown( "cvpcb.Control.panDown",
        AS_GLOBAL, MD_SHIFT + WXK_DOWN, "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );
TOOL_ACTION CVPCB_ACTIONS::panLeft( "cvpcb.Control.panLeft",
        AS_GLOBAL, MD_SHIFT + WXK_LEFT, "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );
TOOL_ACTION CVPCB_ACTIONS::panRight( "cvpcb.Control.panRight",
        AS_GLOBAL, MD_SHIFT + WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );

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


// Cursor control
int CVPCB_CONTROL::CursorControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    bool fastMove = type & CVPCB_ACTIONS::CURSOR_FAST_MOVE;
    type &= ~CVPCB_ACTIONS::CURSOR_FAST_MOVE;
    bool mirroredX = getView()->IsMirroredX();

    GRID_HELPER gridHelper( m_frame );
    VECTOR2D cursor = getViewControls()->GetRawCursorPosition( true );
    VECTOR2I gridSize = gridHelper.GetGrid();

    if( fastMove )
        gridSize = gridSize * 10;

    switch( type )
    {
        case CVPCB_ACTIONS::CURSOR_UP:
            cursor -= VECTOR2D( 0, gridSize.y );
            break;

        case CVPCB_ACTIONS::CURSOR_DOWN:
            cursor += VECTOR2D( 0, gridSize.y );
            break;

        case CVPCB_ACTIONS::CURSOR_LEFT:
            cursor -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case CVPCB_ACTIONS::CURSOR_RIGHT:
            cursor += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case CVPCB_ACTIONS::CURSOR_CLICK:              // fall through
        case CVPCB_ACTIONS::CURSOR_DBL_CLICK:
        {
            TOOL_ACTIONS action = TA_NONE;
            int modifiers = 0;

            modifiers |= wxGetKeyState( WXK_SHIFT ) ? MD_SHIFT : 0;
            modifiers |= wxGetKeyState( WXK_CONTROL ) ? MD_CTRL : 0;
            modifiers |= wxGetKeyState( WXK_ALT ) ? MD_ALT : 0;

            if( type == CVPCB_ACTIONS::CURSOR_CLICK )
                action = TA_MOUSE_CLICK;
            else if( type == CVPCB_ACTIONS::CURSOR_DBL_CLICK )
                action = TA_MOUSE_DBLCLICK;
            else
                wxFAIL;

            TOOL_EVENT evt( TC_MOUSE, action, BUT_LEFT | modifiers );
            evt.SetMousePosition( getViewControls()->GetCursorPosition() );
            m_toolMgr->ProcessEvent( evt );

            return 0;
        }
        break;
    }

    getViewControls()->SetCursorPosition( cursor );

    return 0;
}


int CVPCB_CONTROL::PanControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    KIGFX::VIEW* view = getView();
    GRID_HELPER gridHelper( m_frame );
    VECTOR2D center = view->GetCenter();
    VECTOR2I gridSize = gridHelper.GetGrid() * 10;
    bool mirroredX = view->IsMirroredX();

    switch( type )
    {
        case CVPCB_ACTIONS::CURSOR_UP:
            center -= VECTOR2D( 0, gridSize.y );
            break;

        case CVPCB_ACTIONS::CURSOR_DOWN:
            center += VECTOR2D( 0, gridSize.y );
            break;

        case CVPCB_ACTIONS::CURSOR_LEFT:
            center -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case CVPCB_ACTIONS::CURSOR_RIGHT:
            center += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        default:
            wxFAIL;
            break;
    }

    view->SetCenter( center );

    return 0;
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
    // Cursor control
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorUp.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorDown.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorLeft.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorRight.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorUpFast.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorDownFast.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorLeftFast.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorRightFast.MakeEvent() );
    Go( &CVPCB_CONTROL::CursorControl,      CVPCB_ACTIONS::cursorClick.MakeEvent() );

    // Pan control
    Go( &CVPCB_CONTROL::PanControl,         CVPCB_ACTIONS::panUp.MakeEvent() );
    Go( &CVPCB_CONTROL::PanControl,         CVPCB_ACTIONS::panDown.MakeEvent() );
    Go( &CVPCB_CONTROL::PanControl,         CVPCB_ACTIONS::panLeft.MakeEvent() );
    Go( &CVPCB_CONTROL::PanControl,         CVPCB_ACTIONS::panRight.MakeEvent() );

    // Miscellaneous
    Go( &CVPCB_CONTROL::ResetCoords,        CVPCB_ACTIONS::resetCoords.MakeEvent() );
    Go( &CVPCB_CONTROL::SwitchCursor,       CVPCB_ACTIONS::switchCursor.MakeEvent() );
    Go( &CVPCB_CONTROL::SwitchUnits,        CVPCB_ACTIONS::switchUnits.MakeEvent() );
}

/*
void CVPCB_CONTROL::updateGrid()
{
    BASE_SCREEN* screen = m_frame->GetScreen();
    //GRID_TYPE grid = screen->GetGrid( idx );
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}
*/