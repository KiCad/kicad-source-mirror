/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCB_DIFF_MERGE_APPLIER_HELPERS_H
#define PCB_DIFF_MERGE_APPLIER_HELPERS_H

#include <board.h>
#include <board_item.h>

#include <kiid.h>

#include <set>


namespace KICAD_DIFF
{

/**
 * Insert every top-level item UUID from @p aBoard into @p aOut.
 *
 * "Top-level" excludes footprint children (pads, fields, graphical items,
 * zones) -- those are handled implicitly when the parent footprint is cloned
 * by the applier.  Promoted from the per-applier lambda so other PCB-side
 * merge tooling (auto-resolution, planner) can share the same walk without
 * re-implementing the const-cast dance over BOARD::GetItemSet.
 */
inline void CollectTopLevelIds( const BOARD* aBoard, std::set<KIID>& aOut )
{
    if( !aBoard )
        return;

    for( const BOARD_ITEM* item : aBoard->GetItemSet() )
    {
        if( item )
            aOut.insert( item->m_Uuid );
    }
}

} // namespace KICAD_DIFF

#endif // PCB_DIFF_MERGE_APPLIER_HELPERS_H
