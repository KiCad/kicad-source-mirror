/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "common.h"
#include "gerbview.h"
#include "class_gerber_draw_item.h"

/* localize a gerber item and return a pointer to it.
 * Display info about this item
 */
GERBER_DRAW_ITEM* GERBVIEW_FRAME::Locate( const wxPoint& aPosition, int aTypeloc )
{
    m_messagePanel->EraseMsgBox();
    wxPoint ref = aPosition;
    bool found = false;

    if( aTypeloc == CURSEUR_ON_GRILLE )
        ref = GetScreen()->GetNearestGridPosition( ref );

    int layer = getActiveLayer();

    // Search first on active layer
    BOARD_ITEM* item = GetBoard()->m_Drawings;
    GERBER_DRAW_ITEM* gerb_item = NULL;

    for( ; item; item = item->Next() )
    {
        gerb_item = (GERBER_DRAW_ITEM*) item;

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
        item = GetBoard()->m_Drawings;

        for( ; item; item = item->Next() )
        {
            gerb_item = (GERBER_DRAW_ITEM*) item;

            if( gerb_item->HitTest( ref ) )
            {
                found = true;
                break;
            }
        }
    }

    if( found )
    {
        gerb_item->DisplayInfo( this );
        return gerb_item;
    }

    return NULL;
}
