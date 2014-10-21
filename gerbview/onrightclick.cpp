/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <id.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <menus_helpers.h>


/* Prepare the right-click pullup menu.
 * The menu already has a list of zoom commands.
 */
bool GERBVIEW_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    GERBER_DRAW_ITEM* DrawStruct = (GERBER_DRAW_ITEM*) GetScreen()->GetCurItem();
    wxString    msg;
    bool        BlockActive = !GetScreen()->m_BlockLocate.IsIdle();
    bool        busy = DrawStruct && DrawStruct->GetFlags();

    // Do not initiate a start block validation on menu.
    m_canvas->SetCanStartBlock( -1 );

    // Simple location of elements where possible.
    if( !busy )
    {
        DrawStruct = Locate( aPosition, CURSEUR_OFF_GRILLE );
        busy = DrawStruct && DrawStruct->GetFlags();
    }

    // If command in progress, end command.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( busy )
            AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), KiBitmap( cancel_xpm )  );
        else
            AddMenuItem( PopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), KiBitmap( cursor_xpm ) );

        PopMenu->AppendSeparator();
    }
    else
    {
        if( busy || BlockActive )
        {
            if( BlockActive )
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel Block" ), KiBitmap( cancel_xpm ) );
                PopMenu->AppendSeparator();
                AddMenuItem( PopMenu, ID_POPUP_PLACE_BLOCK,
                             _( "Place Block" ), KiBitmap( checked_ok_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_DELETE_BLOCK,
                             _( "Delete Block (ctrl + drag mouse)" ), KiBitmap( delete_xpm ) );
            }
            else
            {
                AddMenuItem( PopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), KiBitmap( cancel_xpm ) );
            }

            PopMenu->AppendSeparator();
        }
    }

    if( BlockActive )
        return true;

    if( DrawStruct )
        GetScreen()->SetCurItem( DrawStruct );

    return true;
}
