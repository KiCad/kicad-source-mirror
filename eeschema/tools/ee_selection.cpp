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

#include <limits>
#include <functional>
#include <tools/ee_selection.h>
#include <sch_item.h>
#include <lib_item.h>

EDA_ITEM* EE_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
    EDA_ITEM* topLeftItem = nullptr;
    wxPoint   topLeftPos;
    wxPoint   pos;

    // find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item in the selection
    for( auto item : m_items )
    {
        SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );
        LIB_ITEM* lib_item = dynamic_cast<LIB_ITEM*>( item );

        if( sch_item )
            pos = sch_item->GetPosition();
        else if( lib_item )
            pos = lib_item->GetPosition();

        if( ( topLeftItem == nullptr )
            || ( pos.x < topLeftPos.x )
            || ( topLeftPos.x == pos.x && pos.y < topLeftPos.y ) )
        {
            topLeftItem = item;
            topLeftPos = pos;
        }
    }

    return static_cast<EDA_ITEM*>( topLeftItem );
}
