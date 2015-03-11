/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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
#include <common.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <class_GERBER.h>
#include <dialog_helpers.h>
#include <class_DCodeSelectionbox.h>

/* Process the command triggered by the left button of the mouse
 * currently: just display info in the message panel.
 */
void GERBVIEW_FRAME::OnLeftClick( wxDC* DC, const wxPoint& aPosition )
{
    GERBER_DRAW_ITEM* DrawStruct = Locate( aPosition, CURSEUR_OFF_GRILLE );

    GetScreen()->SetCurItem( DrawStruct );

    if( DrawStruct == NULL )
    {
        GERBER_IMAGE* gerber = g_GERBER_List.GetGbrImage( getActiveLayer() );

        if( gerber )
            gerber->DisplayImageInfo( );
    }
}


/* Called on a double click of left mouse button.
 */
void GERBVIEW_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& aPosition )
{
    // Currently: no nothing
}
