/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <tools/pl_selection.h>
#include <ws_draw_item.h>


EDA_ITEM* PL_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
    WS_DRAW_ITEM_BASE* topLeftItem = nullptr;
    EDA_RECT           topLeftItemBB;

    // find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item in the selection
    for( auto item : m_items )
    {
        WS_DRAW_ITEM_BASE* currentItem = static_cast<WS_DRAW_ITEM_BASE*>( item );
        EDA_RECT currentItemBB = currentItem->GetBoundingBox();

        if( topLeftItem == nullptr )
        {
            topLeftItem = currentItem;
            topLeftItemBB = currentItemBB;
        }
        else if( currentItemBB.GetLeft() < topLeftItemBB.GetLeft() )
        {
            topLeftItem = currentItem;
            topLeftItemBB = currentItemBB;
        }
        else if( topLeftItemBB.GetLeft() == currentItemBB.GetLeft()
                    && currentItemBB.GetTop() < topLeftItemBB.GetTop() )
        {
            topLeftItem = currentItem;
            topLeftItemBB = currentItemBB;
        }
    }

    return static_cast<EDA_ITEM*>( topLeftItem );
}

