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
    bool    keyHandled = false;
    wxPoint pos = aPosition;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    if( aHotKey )
        keyHandled = GeneralControlKeyMovement( aHotKey, &pos, true );

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_canvas->CrossHairOff( aDC );
    else
        m_canvas->CrossHairOn( aDC );

    GetGalCanvas()->GetViewControls()->SetSnapping( false );
    SetCrossHairPosition( pos, false );

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( aDC, aPosition, true );

    if( aHotKey && OnHotKey( aDC, aHotKey, aPosition, NULL ) )
        keyHandled = true;

    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    ClearSearchTreeSelection();

    UpdateStatusBar();

    return keyHandled;
}

