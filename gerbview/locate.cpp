/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file locate.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <msgpanel.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_draw_item.h>


/* localize a gerber item and return a pointer to it.
 * Display info about this item
 */
GERBER_DRAW_ITEM* GERBVIEW_FRAME::Locate( const wxPoint& aPosition, int aTypeloc )
{
    m_messagePanel->EraseMsgBox();
    wxPoint ref = aPosition;
    bool found = false;

    if( aTypeloc == CURSEUR_ON_GRILLE )
        ref = GetNearestGridPosition( ref );

    int layer = getActiveLayer();

    // Search first on active layer
    GERBER_DRAW_ITEM* gerb_item = GetItemsList();

    for( ; gerb_item; gerb_item = gerb_item->Next() )
    {
        if( gerb_item->GetLayer()!= layer )
            continue;

        if( gerb_item->HitTest( ref ) )
        {
            found = true;
            break;
        }
    }

    if( !found ) // Search on all layers
    {
        for( gerb_item = GetItemsList(); gerb_item; gerb_item = gerb_item->Next() )
        {
            if( gerb_item->HitTest( ref ) )
            {
                found = true;
                break;
            }
        }
    }

    if( found )
    {
        MSG_PANEL_ITEMS items;
        gerb_item->GetMsgPanelInfo( items );
        SetMsgPanel( items );
        return gerb_item;
    }

    return NULL;
}
