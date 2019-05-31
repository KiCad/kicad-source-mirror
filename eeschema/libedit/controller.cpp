/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <eda_dde.h>
#include <sch_edit_frame.h>
#include <menus_helpers.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <eeschema_id.h>
#include <general.h>
#include <hotkeys.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <lib_draw_item.h>
#include <lib_pin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_view.h>

bool LIB_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey )
{
    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    // when moving mouse, use the "magnetic" grid, unless the shift+ctrl keys is pressed
    // for next cursor position
    // ( shift or ctrl key down are PAN command with mouse wheel)
    bool snapToGrid = true;

    if( !aHotKey && wxGetKeyState( WXK_SHIFT ) && wxGetKeyState( WXK_CONTROL ) )
        snapToGrid = false;

    // Cursor is left off grid only if no block in progress
    if( GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        snapToGrid = true;

    wxPoint pos = aPosition;
    bool keyHandled = GeneralControlKeyMovement( aHotKey, &pos, snapToGrid );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_canvas->CrossHairOff( aDC );
    else
        m_canvas->CrossHairOn( aDC );

    GetGalCanvas()->GetViewControls()->SetSnapping( snapToGrid );
    SetCrossHairPosition( pos, snapToGrid );

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( aDC, aPosition, true );

    if( aHotKey )
    {
        if( m_movingCursorWithKeyboard )    // The hotkey was a move crosshair cursor command
        {
            // The crosshair was moved. move the mouse cursor to the new crosshair position:
            GetGalCanvas()->GetViewControls()->WarpCursor( GetCrossHairPosition(), true );
            m_movingCursorWithKeyboard = 0;
            keyHandled = true;
        }
        else
        {
            keyHandled = OnHotKey( aDC, aHotKey, aPosition, NULL );
        }
    }



    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    ClearSearchTreeSelection();

    UpdateStatusBar();

    return keyHandled;
}

